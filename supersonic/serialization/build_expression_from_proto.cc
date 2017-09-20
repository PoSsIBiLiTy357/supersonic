// Copyright 2010 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "supersonic/serialization/build_expression_from_proto.h"

#include <cstddef>
#include <cstdint>

#include <memory>
#include <string>
namespace supersonic {using std::string; }
#include <vector>
using std::vector;

#include "supersonic/utils/integral_types.h"
#include "supersonic/utils/stringprintf.h"
#include "supersonic/utils/exception/coowned_pointer.h"
#include "supersonic/utils/exception/failureor.h"
#include "supersonic/utils/proto/expressions.pb.h"
#include "supersonic/utils/proto/types.pb.h"
#include "supersonic/base/exception/exception.h"
#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/base/infrastructure/tuple_schema.h"
#include "supersonic/base/infrastructure/types_infrastructure.h"
#include "supersonic/expression/base/expression.h"
#include "supersonic/expression/core/arithmetic_expressions.h"
#include "supersonic/expression/core/comparison_expressions.h"
#include "supersonic/expression/core/date_expressions.h"
#include "supersonic/expression/core/elementary_expressions.h"
#include "supersonic/expression/core/math_expressions.h"
#include "supersonic/expression/core/projecting_expressions.h"
#ifdef HAVE_RE2_RE2_H
#include "supersonic/expression/core/regexp_expressions.h"
#endif  // HAVE_RE2_RE2_H
#include "supersonic/expression/core/string_expressions.h"
#include "supersonic/expression/infrastructure/terminal_expressions.h"
#include "supersonic/expression/templated/bound_expression_factory.h"
#include "supersonic/proto/supersonic.pb.h"
#include <google/protobuf/repeated_field.h>
#include "supersonic/utils/strings/join.h"
#include "supersonic/utils/pointer_vector.h"

namespace supersonic {

using util::gtl::PointerVector;

namespace {

using common::Constant;
using common::CustomFunctionCall;
using common::ExpressionDescription;
using common::ExpressionType;
using common::OperationDescription;
using common::OperationType;
using common::Variable;
using common::Tuple;

typedef FailureOrOwned<const Expression> ExpressionResult;

FailureOr<DataType> ResolveType(const common::DataType& type) {
  if (type >= 0 && type <= 11) {
    return Success(static_cast<DataType>(type));
  } else {
    return Failure(
        new Exception(ERROR_NOT_IMPLEMENTED,
                      StrCat("Type ",
                             common::DataType_Name(type),
                             " not supported in Supersonic")));
  }
}

template<DataType type> struct ExtractConstant {};

template<> struct ExtractConstant<INT32> {
  static int32 Value(const Constant& c) { return c.int32_value(); }
  static bool HasValue(const Constant& c) { return c.has_int32_value(); }
};

template<> struct ExtractConstant<INT64> {
  static int64 Value(const Constant& c) { return c.int64_value(); }
  static bool HasValue(const Constant& c) { return c.has_int64_value(); }
};

template<> struct ExtractConstant<UINT32> {
  static uint32 Value(const Constant& c) { return c.uint32_value(); }
  static bool HasValue(const Constant& c) { return c.has_uint32_value(); }
};

template<> struct ExtractConstant<UINT64> {
  static uint64 Value(const Constant& c) { return c.uint64_value(); }
  static bool HasValue(const Constant& c) { return c.has_uint64_value(); }
};

template<> struct ExtractConstant<FLOAT> {
  static float Value(const Constant& c) { return c.float_value(); }
  static bool HasValue(const Constant& c) { return c.has_float_value(); }
};

template<> struct ExtractConstant<DOUBLE> {
  static double Value(const Constant& c) { return c.double_value(); }
  static bool HasValue(const Constant& c) { return c.has_double_value(); }
};

template<> struct ExtractConstant<DATE> {
  static int32 Value(const Constant& c) { return c.date_value(); }
  static bool HasValue(const Constant& c) { return c.has_date_value(); }
};

template<> struct ExtractConstant<DATETIME> {
  static int64 Value(const Constant& c) { return c.datetime_value(); }
  static bool HasValue(const Constant& c) { return c.has_datetime_value(); }
};

template<> struct ExtractConstant<BOOL> {
  static bool Value(const Constant& c) { return c.bool_value(); }
  static bool HasValue(const Constant& c) { return c.has_bool_value(); }
};

template<> struct ExtractConstant<STRING> {
  static string Value(const Constant& c) { return c.string_value(); }
  static bool HasValue(const Constant& c) { return c.has_string_value(); }
};

template<> struct ExtractConstant<BINARY> {
  static string Value(const Constant& c) { return c.binary_value(); }
  static bool HasValue(const Constant& c) { return c.has_binary_value(); }
};

template<DataType type>
ExpressionResult CreateConstExpression(const Constant& c) {
  return Success(
      ExtractConstant<type>::HasValue(c)
          ? TypedConst<type>(ExtractConstant<type>::Value(c))
          : Null(type));
}

// Special case for 'DataType' constants; we need to check the value to
// make sure it's supported by Supersonic.
template<>
ExpressionResult CreateConstExpression<DATA_TYPE>(const Constant& c) {
  if (c.has_data_type_value()) {
    FailureOr<DataType> resolved = ResolveType(c.data_type_value());
    PROPAGATE_ON_FAILURE(resolved);
    return Success(TypedConst<DATA_TYPE>(resolved.get()));
  } else {
    return Success(Null(DATA_TYPE));
  }
}

template<>
ExpressionResult CreateConstExpression<ENUM>(const Constant& c) {
  THROW(new Exception(
      ERROR_NOT_IMPLEMENTED,
      "ENUM constants are not currently supported. (It's challenging to "
      "define a good format to specify the values, in which type information "
      "would be separated from data - and e.g. shared between multiple "
      "constants."));
}

struct ConstExpressionFactory {
  explicit ConstExpressionFactory(const Constant& constant)
      : constant(constant) {}
  template<DataType type> ExpressionResult operator()() const {
    return CreateConstExpression<type>(constant);
  }
  const Constant& constant;
};

}  // namespace

ExpressionResult BuildConstantFromProto(const Constant& constant) {
  FailureOr<DataType> type = ResolveType(constant.type());
  PROPAGATE_ON_FAILURE(type);
  ConstExpressionFactory factory(constant);
  return TypeSpecialization<ExpressionResult, ConstExpressionFactory>(
      type.get(), factory);
}

ExpressionResult BuildVariableFromProto(const Variable& var_descr) {
  return Success(NamedAttribute(var_descr.name()));
}

string WrongNumberOfArgumentsString(int expected, int received) {
  return StringPrintf(
      "Wrong number of arguments to operation, "
      "expected: %d, while received: %d.",
      expected, received);
}

// Helper functions for building unary, binary and ternary operations with a
// check for the correct number of arguments in the proto.
static ExpressionResult MakeUnaryOperation(
    unique_ptr<const Expression> (*function) (unique_ptr<const Expression>),
    vector<unique_ptr<const Expression>> args) {
  if (args.size() != 1) {
    THROW(new Exception(
        ERROR_BAD_PROTO,
        WrongNumberOfArgumentsString(1, args.size())));
  }
  return Success(function(std::move(args[0])));
}

static ExpressionResult MakeBinaryOperation(
    unique_ptr<const Expression> (*function) (
        unique_ptr<const Expression>, unique_ptr<const Expression>),
    vector<unique_ptr<const Expression>> args) {
  if (args.size() != 2) {
    THROW(new Exception(
        ERROR_BAD_PROTO,
        WrongNumberOfArgumentsString(2, args.size())));
  }
  return Success(function(std::move(args[0]),
                          std::move(args[1])));
}

static ExpressionResult MakeTernaryOperation(
    unique_ptr<const Expression> (*function) (
        unique_ptr<const Expression>, unique_ptr<const Expression>,
        unique_ptr<const Expression>),
    vector<unique_ptr<const Expression>> args) {
  if (args.size() != 3) {
    THROW(new Exception(
        ERROR_BAD_PROTO,
        WrongNumberOfArgumentsString(3, args.size())));
  }
  return Success(function(std::move(args[0]),
                          std::move(args[1]),
                          std::move(args[2])));
}

static ExpressionResult MakeSenaryOperation(
    unique_ptr<const Expression> (*function) (
        unique_ptr<const Expression>, unique_ptr<const Expression>,
        unique_ptr<const Expression>, unique_ptr<const Expression>,
        unique_ptr<const Expression>, unique_ptr<const Expression>),
    vector<unique_ptr<const Expression>> args) {
  if (args.size() != 6) {
    THROW(new Exception(
        ERROR_BAD_PROTO,
        WrongNumberOfArgumentsString(6, args.size())));
  }
  return Success(function(std::move(args[0]),
                          std::move(args[1]),
                          std::move(args[2]),
                          std::move(args[3]),
                          std::move(args[4]),
                          std::move(args[5])));
}

static ExpressionResult MakeNaryOperation(
    unique_ptr<const Expression> (*function) (unique_ptr<const ExpressionList>),
    vector<unique_ptr<const Expression>> args) {

  if (args.size() == 0) {
    THROW(new Exception(
        ERROR_BAD_PROTO,
        "Wrong number of arguments to operation, expected positive number, "
        "received zero."));
  }

  auto list = make_unique<ExpressionList>();
  for (int i = 0; i < args.size(); ++i) {
    list->add(std::move(args[i]));
  }
  return Success(function(std::move(list)));
}

// In addition to the helper functions that include arity explicitly in their
// name (Make[Arity]Operation), we also overload MakeOperation for different
// function pointer types. This is not really essential, but makes life a bit
// easier.
static ExpressionResult MakeOperation(
    unique_ptr<const Expression> (*function) (),
    vector<unique_ptr<const Expression>> args) {
  if (args.size() != 0) {
    THROW(new Exception(
        ERROR_BAD_PROTO,
        WrongNumberOfArgumentsString(0, args.size())));
  }
  return Success(function());
}

static ExpressionResult MakeOperation(
    unique_ptr<const Expression> (*function) (unique_ptr<const Expression>),
    vector<unique_ptr<const Expression>> args) {
  return MakeUnaryOperation(function, std::move(args));
}

static ExpressionResult MakeOperation(
    unique_ptr<const Expression> (*function) (
        unique_ptr<const Expression>, unique_ptr<const Expression>),
    vector<unique_ptr<const Expression>>args) {
  return MakeBinaryOperation(function, std::move(args));
}

static ExpressionResult MakeOperation(
    unique_ptr<const Expression> (*function) (
        unique_ptr<const Expression>, unique_ptr<const Expression>,
        unique_ptr<const Expression>),
    vector<unique_ptr<const Expression>> args) {
  return MakeTernaryOperation(function, std::move(args));
}

static ExpressionResult MakeOperation(
    unique_ptr<const Expression> (*function) (
        unique_ptr<const Expression>, unique_ptr<const Expression>,
        unique_ptr<const Expression>, unique_ptr<const Expression>,
        unique_ptr<const Expression>, unique_ptr<const Expression>),
    vector<unique_ptr<const Expression>> args) {
  return MakeSenaryOperation(function, std::move(args));
}

static ExpressionResult MakeOperation(
    unique_ptr<const Expression> (*function) (unique_ptr<const ExpressionList>),
    vector<unique_ptr<const Expression>> args) {
  return MakeNaryOperation(function, std::move(args));
}

ExpressionResult OperatorNotImplemented(OperationType op_type) {
  THROW(new Exception(
      ERROR_NOT_IMPLEMENTED,
      "BuildOperationFromProto: Operator " +
      common::OperationType_Name(op_type) +
      " not implemented yet."));
}

template<DataType type, typename CppType>
std::vector<CppType> BuildConstantsVectorFromProto(
    std::vector<Constant>& consts) {
  std::vector<CppType> values(consts.size());
  for (int i = 0; i < consts.size(); ++i) {
    if (ExtractConstant<type>::HasValue(consts[i])) {
      values[i] = ExtractConstant<type>::Value(consts[i]);
    }
  }
  return values;
}

ExpressionResult MakeRegexpOperation(const OperationDescription& regexp_descr) {
#ifdef HAVE_RE2_RE2_H
  if (regexp_descr.argument_size() != 2) {
    THROW(new Exception(
        ERROR_BAD_PROTO,
        StrCat("Wrong number of arguments for REGEXP. Expected exactly two; "
               "given ", regexp_descr.argument_size(), ": ",
               regexp_descr.DebugString())));
  }
  ExpressionResult result = BuildExpressionFromProto(regexp_descr.argument(0));
  PROPAGATE_ON_FAILURE(result);

  if (!regexp_descr.argument(1).has_constant()) {
    THROW(new Exception(
        ERROR_NOT_IMPLEMENTED,
        StrCat("REGEXP supports only constant patterns in expression: ",
               regexp_descr.DebugString())));
  }
  const Constant& c = regexp_descr.argument(1).constant();
  if (c.type() != common::STRING) {
    THROW(new Exception(
        ERROR_ATTRIBUTE_TYPE_MISMATCH,
        StrCat("Pattern in REGEXP should be a STRING in expression: ",
               regexp_descr.DebugString())));
  }
  switch (regexp_descr.type()) {
    case common::REGEXP_FULL:
      return Success(RegexpFullMatch(result.move(), c.string_value()));
    case common::REGEXP_PARTIAL:
      return Success(RegexpPartialMatch(result.move(), c.string_value()));
    case common::REGEXP_EXTRACT:
      return Success(RegexpExtract(result.move(), c.string_value()));
    default:
      LOG(FATAL) << "Unknown type of expression: "
                 << common::OperationType_Name(regexp_descr.type())
                 << " passed over to MakeRegexpOperation. Should be one of "
                 << "REGEXP_FULL, REGEXP_PARTIAL or REGEXP_EXTRACT.";
  }
#else
  THROW(new Exception(
        ERROR_BAD_PROTO,
        "Supersonic complied without re2. Regexp expressions not supported."));
#endif
}

ExpressionResult MakeRegexpReplaceOperation(
    const OperationDescription& regexp_descr) {
#ifdef HAVE_RE2_RE2_H
  if (regexp_descr.argument_size() != 3) {
    THROW(new Exception(
        ERROR_BAD_PROTO,
        StrCat("Wrong number of arguments for REGEXP_REPLACE. Expected exactly "
               "three; given ", regexp_descr.argument_size(), ": ",
               regexp_descr.DebugString())));
  }
  ExpressionResult left = BuildExpressionFromProto(regexp_descr.argument(0));
  // No context, as the error actually occurred in the child.
  PROPAGATE_ON_FAILURE(left);

  if (!regexp_descr.argument(1).has_constant()) {
    THROW(new Exception(
        ERROR_NOT_IMPLEMENTED,
        StrCat("REGEXP_REPLACE supports only constant patterns in ",
               regexp_descr.DebugString())));
  }
  const Constant& c = regexp_descr.argument(1).constant();
  if (c.type() != common::STRING) {
    THROW(new Exception(
        ERROR_ATTRIBUTE_TYPE_MISMATCH,
        StrCat("Pattern in REGEXP should be a STRING in expression: ",
               regexp_descr.DebugString())));
  }

  ExpressionResult right = BuildExpressionFromProto(regexp_descr.argument(2));
  // No context, as the error actually occurred in the child.
  PROPAGATE_ON_FAILURE(right);

  return Success(RegexpReplace(left.move(), c.string_value(),
                               right.move()));
#else
  THROW(new Exception(
        ERROR_BAD_PROTO,
        "Supersonic complied without re2. Regexp expressions not supported."));
#endif
}

ExpressionResult MakeInOperation(const OperationDescription& in_description) {
  if (in_description.argument_size() == 0) {
    THROW(new Exception(
        ERROR_BAD_PROTO,
        StrCat("Wrong number of arguments for IN - at least one expected ",
               "in expression", in_description.DebugString())));
  }

  ExpressionResult needle_result =
      BuildExpressionFromProto(in_description.argument(0));
  // No context here, as the error actually occurred in the child.
  PROPAGATE_ON_FAILURE(needle_result);
  auto haystack = make_unique<ExpressionList>();
  for (int i = 1; i < in_description.argument_size(); ++i) {
    ExpressionResult child_result =
        BuildExpressionFromProto(in_description.argument(i));
    PROPAGATE_ON_FAILURE(child_result);
    haystack->add(child_result.move());
  }
  return Success(In(needle_result.move(), std::move(haystack)));
}

ExpressionResult MakeCastOperation(
    const OperationDescription& cast_descr) {
  if (cast_descr.argument_size() != 2) {
    THROW(new Exception(
        ERROR_BAD_PROTO,
        StrCat("Wrong number of arguments for CAST - exactly two expected ",
               "in expression", cast_descr.DebugString())));
  }
  if (cast_descr.argument(0).type() != common::CONSTANT
      || !cast_descr.argument(0).has_constant()
      || cast_descr.argument(0).constant().type() != common::DATA_TYPE) {
    THROW(new Exception(
        ERROR_BAD_PROTO,
        StrCat("Wrong first argument for CAST - data type constant expected ",
               "in expression ", cast_descr.DebugString())));
  }
  FailureOr<DataType> castType =
      ResolveType(cast_descr.argument(0).constant().data_type_value());
  PROPAGATE_ON_FAILURE_WITH_CONTEXT(castType, cast_descr.DebugString(), "");

  ExpressionResult expressionToCast =
      BuildExpressionFromProto(cast_descr.argument(1));
  // No context here, as the error actually occurred in the child.
  PROPAGATE_ON_FAILURE(expressionToCast);

  return Success(CastTo(castType.get(), expressionToCast.move()));
}

ExpressionResult MakeParseOperation(
    const OperationDescription& parse_description) {
  if (parse_description.argument_size() != 2) {
    string error = StrCat(
        "Wrong number of arguments for PARSE_STRING - exactly two expected ",
        "(type parsed to and input column). Received: ",
        parse_description.argument_size(), " in expression: ",
        parse_description.DebugString());
    THROW(new Exception(ERROR_BAD_PROTO, error));
  }
  if (parse_description.argument(0).type() != common::CONSTANT
      || !parse_description.argument(0).has_constant()
      || parse_description.argument(0).constant().type() != common::DATA_TYPE) {
    THROW(new Exception(
        ERROR_BAD_PROTO,
        StrCat("Wrong first argument for PARSE_STRING, ",
               "data type constant expected in expression: ",
               parse_description.DebugString())));
  }
  FailureOr<DataType> parseToType =
      ResolveType(parse_description.argument(0).constant().data_type_value());
  PROPAGATE_ON_FAILURE_WITH_CONTEXT(parseToType,
                                    parse_description.DebugString(),
                                    "");

  ExpressionResult expressionToParse =
      BuildExpressionFromProto(parse_description.argument(1));
  // No context here, as the error actually occurred in the child.
  PROPAGATE_ON_FAILURE(expressionToParse);

  return Success(ParseStringNulling(parseToType.get(),
                                    expressionToParse.move()));
}

ExpressionResult BuildStandardOperationFromProto(
    OperationType op_type,
    vector<unique_ptr<const Expression>> args) {
  switch (op_type) {
    // Expressions already handled above, this is to avoid a compile-time
    // warning.
    case common::IN:
    case common::CAST:
    case common::PARSE_STRING:
    case common::REGEXP_FULL:
    case common::REGEXP_PARTIAL:
    case common::REGEXP_EXTRACT:
    case common::REGEXP_REPLACE:
        LOG(FATAL) << "An expression that should have been handled before "
                   << "was passed to the `standard' case of the "
                   << "BuildOperationFromProto function: "
                   << common::OperationType_Name(op_type);
    case common::ADD: return MakeOperation(&Plus, std::move(args));
    case common::MULTIPLY: return MakeOperation(&Multiply, std::move(args));
    case common::SUBTRACT: return MakeOperation(&Minus, std::move(args));
    case common::DIVIDE: return MakeOperation(&Divide, std::move(args));
    case common::DIVIDE_SIGNALING:
        return MakeOperation(&DivideSignaling, std::move(args));
    case common::DIVIDE_NULLING: return MakeOperation(&DivideNulling, std::move(args));
    case common::DIVIDE_QUIET: return MakeOperation(&DivideQuiet, std::move(args));
    case common::CPP_DIVIDE: return MakeOperation(&CppDivide, std::move(args));
    case common::MODULUS: return MakeOperation(&Modulus, std::move(args));
    case common::IS_ODD: return MakeOperation(&IsOdd, std::move(args));
    case common::IS_EVEN: return MakeOperation(&IsEven, std::move(args));
    case common::NEGATE: return MakeOperation(&Negate, std::move(args));
    case common::NOT: return MakeOperation(&Not, std::move(args));
    case common::AND: return MakeOperation(&And, std::move(args));
    case common::OR: return MakeOperation(&Or, std::move(args));
    case common::AND_NOT: return MakeOperation(&AndNot, std::move(args));
    case common::XOR: return MakeOperation(&Xor, std::move(args));
    case common::EQUAL: return MakeOperation(&Equal, std::move(args));
    case common::NOT_EQUAL: return MakeOperation(&NotEqual, std::move(args));
    case common::GREATER: return MakeOperation(&Greater, std::move(args));
    case common::GREATER_OR_EQUAL: return MakeOperation(&GreaterOrEqual, std::move(args));
    case common::LESS: return MakeOperation(&Less, std::move(args));
    case common::LESS_OR_EQUAL: return MakeOperation(&LessOrEqual, std::move(args));
    case common::IS_NULL: return MakeOperation(&IsNull, std::move(args));
    case common::IFNULL: return MakeOperation(&IfNull, std::move(args));
    case common::IF: return MakeOperation(&If, std::move(args));
    case common::CASE: return MakeOperation(&Case, std::move(args));
    case common::BITWISE_AND: return MakeOperation(&BitwiseAnd, std::move(args));
    case common::BITWISE_OR: return MakeOperation(&BitwiseOr, std::move(args));
    case common::BITWISE_NOT: return MakeOperation(&BitwiseNot, std::move(args));
    case common::BITWISE_XOR: return MakeOperation(&BitwiseXor, std::move(args));
    case common::SHIFT_LEFT: return MakeOperation(&ShiftLeft, std::move(args));
    case common::SHIFT_RIGHT: return MakeOperation(&ShiftRight, std::move(args));
    case common::COPY: return OperatorNotImplemented(op_type);
    case common::ROUND:
        if (args.size() == 1) return MakeOperation(&Round, std::move(args));
        if (args.size() == 2) return MakeOperation(&RoundWithPrecision, std::move(args));
        THROW(new Exception(
            ERROR_BAD_PROTO,
            StringPrintf("Expected 1 or 2 arguments to ROUND, received %zd.",
                         args.size())));
    case common::ROUND_TO_INT: return MakeOperation(&RoundToInt, std::move(args));
    case common::TRUNC: return MakeOperation(&Trunc, std::move(args));
    case common::CEIL: return MakeOperation(&Ceil, std::move(args));
    case common::CEIL_TO_INT: return MakeOperation(&CeilToInt, std::move(args));
    case common::EXP: return MakeOperation(&Exp, std::move(args));
    case common::FLOOR: return MakeOperation(&Floor, std::move(args));
    case common::FLOOR_TO_INT: return MakeOperation(&FloorToInt, std::move(args));
    case common::LN: return MakeOperation(&Ln, std::move(args));
    case common::LOG10: return MakeOperation(&Log10, std::move(args));
    case common::LOG: return MakeOperation(&Log, std::move(args));
    case common::SQRT: return MakeOperation(&Sqrt, std::move(args));
    case common::SQRT_NULLING: return MakeOperation(&SqrtNulling, std::move(args));
    case common::SQRT_SIGNALING: return MakeOperation(&SqrtSignaling, std::move(args));
    case common::SQRT_QUIET: return MakeOperation(&SqrtQuiet, std::move(args));
    case common::POWER_QUIET: return MakeOperation(&PowerQuiet, std::move(args));
    case common::POWER_SIGNALING: return MakeOperation(&PowerSignaling, std::move(args));
    case common::POWER_NULLING: return MakeOperation(&PowerNulling, std::move(args));
    case common::SIN: return MakeOperation(&Sin, std::move(args));
    case common::COS: return MakeOperation(&Cos, std::move(args));
    case common::TAN: return MakeOperation(&Tan, std::move(args));
    case common::PI: return MakeOperation(&Pi, std::move(args));
    case common::LENGTH: return MakeOperation(&Length, std::move(args));
    case common::LTRIM: return MakeOperation(&Ltrim, std::move(args));
    case common::RTRIM: return MakeOperation(&Rtrim, std::move(args));
    case common::TRIM: return MakeOperation(&Trim, std::move(args));
    case common::TOUPPER: return MakeOperation(&ToUpper, std::move(args));
    case common::TOLOWER: return MakeOperation(&ToLower, std::move(args));
    case common::UNIXTIMESTAMP: return MakeOperation(&UnixTimestamp, std::move(args));
    case common::FROMUNIXTIME: return MakeOperation(&FromUnixTime, std::move(args));
    case common::MAKEDATE: return MakeOperation(&MakeDate, std::move(args));
    case common::MAKEDATETIME: return MakeOperation(&MakeDatetime, std::move(args));
    case common::DATEDIFF: return OperatorNotImplemented(op_type);
    case common::DATETIMEDIFF: return OperatorNotImplemented(op_type);
    case common::ADD_MINUTE:
        if (args.size() == 1) return MakeOperation(&AddMinute, std::move(args));
        if (args.size() == 2) return MakeOperation(&AddMinutes, std::move(args));
        THROW(new Exception(
            ERROR_BAD_PROTO,
            StringPrintf("Expected 1 or 2 arguments to ADD_MINUTE, got %zd.",
                         args.size())));
    case common::ADD_DAY:
        if (args.size() == 1) return MakeOperation(&AddDay, std::move(args));
        if (args.size() == 2) return MakeOperation(&AddDays, std::move(args));
        THROW(new Exception(
            ERROR_BAD_PROTO,
            StringPrintf("Expected 1 or 2 arguments to ADD_DAY, received %zd.",
                         args.size())));
    case common::ADD_WEEK: return OperatorNotImplemented(op_type);
    case common::ADD_MONTH:
        if (args.size() == 1) return MakeOperation(&AddMonth, std::move(args));
        if (args.size() == 2) return MakeOperation(&AddMonths, std::move(args));
        THROW(new Exception(
            ERROR_BAD_PROTO,
            StringPrintf("Expected 1 or 2 arguments to ADD_MONTH, got %zd.",
                         args.size())));
    case common::ADD_YEAR: return OperatorNotImplemented(op_type);
    case common::TRUNC_TO_SECOND: return OperatorNotImplemented(op_type);
    case common::TRUNC_TO_MINUTE: return OperatorNotImplemented(op_type);
    case common::TRUNC_TO_HOUR: return OperatorNotImplemented(op_type);
    case common::TRUNC_TO_DAY: return OperatorNotImplemented(op_type);
    case common::TRUNC_TO_MONTH: return OperatorNotImplemented(op_type);
    case common::TRUNC_TO_QUARTER: return OperatorNotImplemented(op_type);
    case common::TRUNC_TO_YEAR: return OperatorNotImplemented(op_type);
    case common::YEAR_LOCAL: return MakeOperation(&YearLocal, std::move(args));
    case common::QUARTER_LOCAL: return MakeOperation(&QuarterLocal, std::move(args));
    case common::MONTH_LOCAL: return MakeOperation(&MonthLocal, std::move(args));
    case common::DAY_LOCAL: return MakeOperation(&DayLocal, std::move(args));
    case common::WEEKDAY_LOCAL: return MakeOperation(&WeekdayLocal, std::move(args));
    case common::YEARDAY_LOCAL: return MakeOperation(&YearDayLocal, std::move(args));
    case common::HOUR_LOCAL: return MakeOperation(&HourLocal, std::move(args));
    case common::MINUTE_LOCAL: return MakeOperation(&MinuteLocal, std::move(args));
    case common::YEAR_UTC: return MakeOperation(&Year, std::move(args));
    case common::QUARTER_UTC: return MakeOperation(&Quarter, std::move(args));
    case common::MONTH_UTC: return MakeOperation(&Month, std::move(args));
    case common::DAY_UTC: return MakeOperation(&Day, std::move(args));
    case common::WEEKDAY_UTC: return MakeOperation(&Weekday, std::move(args));
    case common::YEARDAY_UTC: return MakeOperation(&YearDay, std::move(args));
    case common::HOUR_UTC: return MakeOperation(&Hour, std::move(args));
    case common::MINUTE_UTC: return MakeOperation(&Minute, std::move(args));
    case common::SECOND: return MakeOperation(&Second, std::move(args));
    case common::MICROSECOND: return MakeOperation(&Microsecond, std::move(args));
    case common::SUBSTRING:
        if (args.size() == 2) return MakeOperation(&TrailingSubstring, std::move(args));
        if (args.size() == 3) return MakeOperation(&Substring, std::move(args));
        THROW(new Exception(
            ERROR_BAD_PROTO,
            StringPrintf("Expected 1 or 2 arguments to SUBSTRING, got %zd.",
                         args.size())));
    case common::FORMAT: return MakeOperation(&Format, std::move(args));
    case common::DATE_FORMAT_LOCAL:
        return MakeOperation(&DateFormatLocal, std::move(args));
    case common::DATE_FORMAT_UTC: return MakeOperation(&DateFormat, std::move(args));
    case common::CONCATENATE: return MakeNaryOperation(&Concat, std::move(args));
    case common::TOSTRING: return MakeOperation(&ToString, std::move(args));
    case common::STRING_OFFSET: return MakeOperation(&StringOffset, std::move(args));
    case common::REPLACE: return MakeOperation(&StringReplace, std::move(args));
    case common::IS_FINITE: return MakeOperation(&IsFinite, std::move(args));
    case common::IS_INF: return MakeOperation(&IsInf, std::move(args));
    case common::IS_NAN: return MakeOperation(&IsNaN, std::move(args));
    case common::IS_NORMAL: return MakeOperation(&IsNormal, std::move(args));
    case common::RANDOM_INT32: return MakeOperation(&RandInt32, std::move(args));
    case common::SEQUENCE: return MakeOperation(&Sequence, std::move(args));

    case common::UNIMPLEMENTED_OPERATOR_0:
        return OperatorNotImplemented(op_type);
    case common::UNIMPLEMENTED_OPERATOR_1:
        return OperatorNotImplemented(op_type);
    case common::UNIMPLEMENTED_OPERATOR_2:
        return OperatorNotImplemented(op_type);
    case common::UNIMPLEMENTED_OPERATOR_3:
        return OperatorNotImplemented(op_type);
    case common::UNIMPLEMENTED_OPERATOR_4:
        return OperatorNotImplemented(op_type);
    case common::UNIMPLEMENTED_OPERATOR_5:
        return OperatorNotImplemented(op_type);
    // There is no default: case handling to force compiler to report
    // "enumeration value 'XX' not handled in switch" for new operation types.
  }
  LOG(FATAL) << "An expression was not handled by the wide switch in "
             << "BuildOperationFromProto: "
             << common::OperationType_Name(op_type);
}

ExpressionResult BuildOperationFromProto(
    const OperationDescription& operation_descr) {
  OperationType op_type = operation_descr.type();
  // Operations that don't want their children evaluated.
  switch (op_type) {
    case common::IN: return MakeInOperation(operation_descr);
    case common::CAST: return MakeCastOperation(operation_descr);
    case common::PARSE_STRING: return MakeParseOperation(operation_descr);
    case common::REGEXP_FULL: return MakeRegexpOperation(operation_descr);
    case common::REGEXP_PARTIAL: return MakeRegexpOperation(operation_descr);
    case common::REGEXP_EXTRACT: return MakeRegexpOperation(operation_descr);
    case common::REGEXP_REPLACE:
        return MakeRegexpReplaceOperation(operation_descr);
    default: break;
  }

  vector<unique_ptr<const Expression>> args;
  for (int i = 0; i < operation_descr.argument_size(); ++i) {
    ExpressionResult child_result =
        BuildExpressionFromProto(operation_descr.argument(i));
    PROPAGATE_ON_FAILURE(child_result);
    args.emplace_back(child_result.move());
  }
  ExpressionResult result = BuildStandardOperationFromProto(op_type, std::move(args));
  PROPAGATE_ON_FAILURE_WITH_CONTEXT(result,
                                    operation_descr.ShortDebugString(),
                                    "");
  return result;
}

ExpressionResult BuildFunctionCallFromProto(
    const common::CustomFunctionCall& function_call_descr) {
  THROW(new Exception(ERROR_NOT_IMPLEMENTED,
                      "BuildFunctionCallFromProto is not implemented yet."));
}

ExpressionResult BuildTupleFromProto(const Tuple& tuple_descr) {
  auto result = make_unique<CompoundExpression>();
  for (int i = 0; i < tuple_descr.expression_size(); i++) {
    const Tuple::TupleExpression& expr = tuple_descr.expression(i);
    ExpressionResult expression = BuildExpressionFromProto(expr.expression());
    if (expression.is_failure()) { return expression; }
    if (expr.alias_size() == 0) {
      result->Add(expression.move());
    } else {
      result->AddAsMulti(
          vector<string>(expr.alias().begin(), expr.alias().end()),
          expression.move());
    }
  }
  return Success(std::move(result));
}

ExpressionResult BuildExpressionFromProto(
    const ExpressionDescription& expression_description) {
  switch (expression_description.type()) {
    case common::CONSTANT:
      if (!expression_description.has_constant()) {
        THROW(new Exception(
            ERROR_BAD_PROTO,
            StrCat("Type set to CONSTANT, but constant field is not set. "
                   "Got: ", expression_description.ShortDebugString())));
      }
      return BuildConstantFromProto(expression_description.constant());
    case common::VARIABLE:
      if (!expression_description.has_variable()) {
        THROW(new Exception(
            ERROR_BAD_PROTO,
            StrCat("Type set to VARIABLE, but variable field is not set. "
                   "Got: ", expression_description.ShortDebugString())));
      }
      return BuildVariableFromProto(expression_description.variable());
    case common::OPERATION:
      if (!expression_description.has_operation()) {
        THROW(new Exception(
            ERROR_BAD_PROTO,
            StrCat("Type set to OPERATION, but operation field is not set. "
                   "Got: ", expression_description.ShortDebugString())));
      }
      return BuildOperationFromProto(expression_description.operation());
    case common::CUSTOM_FUNCTION_CALL:
      if (!expression_description.has_function_call()) {
        THROW(new Exception(
            ERROR_BAD_PROTO,
            StrCat("Type set to CUSTOM_FUNCTION_CALL, but function_call field "
                   "is not set. Got: ",
                   expression_description.ShortDebugString())));
      }
      return BuildFunctionCallFromProto(expression_description.function_call());
    case common::TUPLE:
      if (!expression_description.has_tuple()) {
        THROW(new Exception(
            ERROR_BAD_PROTO,
            StrCat("Type set to TUPLE, but tuple field is not set. "
                   "Got: ", expression_description.ShortDebugString())));
      }
      return BuildTupleFromProto(expression_description.tuple());
    default:
      THROW(new Exception(
          ERROR_BAD_PROTO,
          StrCat("Unknown expression type: ",
                 expression_description.ShortDebugString())));
  }
}

}  // namespace supersonic
