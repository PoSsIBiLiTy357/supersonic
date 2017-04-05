cmake_minimum_required(VERSION 3.2)
project(supersonic)

set(CMAKE_MODULE_PATH 
    ${CMAKE_SOURCE_DIR}/cmake/externals/sanitizers-cmake/cmake
    ${CMAKE_SOURCE_DIR}/cmake/Modules
    ${CMAKE_MODULE_PATH}
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_STANDARD 14)

find_package(Boost 1.20.0 REQUIRED COMPONENTS system timer)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(RE2)
find_package(GFlags REQUIRED)
find_package(Glog REQUIRED)
find_package(Sanitizers)

include(InstallHeaders)

include(ExternalProject)
include(CMakeOptions.txt)

add_subdirectory(external/google-test)

# Minimum instruction set needed
set(INST_SET "-mmmx -msse -msse3 -msse3 -mssse3 -msse4 -msse4a -msse4.1 -msse4.2 -mpopcnt")

set(CMAKE_C_FLAGS "${INST_SET} -funsigned-char")
set(CMAKE_CXX_FLAGS "${INST_SET} -funsigned-char")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og")


FUNCTION (proto_gen OUT_DIR PROTO_FILE)
    get_filename_component(PROTO_DIR ${PROTO_FILE} DIRECTORY)
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(PROTO_OUT_PATH ${OUT_DIR}/${PROTO_DIR})

    add_custom_command(
        PRE_BUILD
        DEPENDS ${CMAKE_SOURCE_DIR}/${PROTO_FILE}
        OUTPUT ${PROTO_OUT_PATH}/${PROTO_NAME}.pb.cc ${PROTO_OUT_PATH}/${PROTO_NAME}.pb.h
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROTO_OUT_PATH}
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} "--cpp_out=${OUT_DIR}" --proto_path=${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/${PROTO_FILE}
    )

    set_source_files_properties(${PROTO_OUT_PATH}/${PROTO_NAME}.pb.cc ${PROTO_OUT_PATH}/${PROTO_NAME}.pb.h
        PROPERTIES GENERATED TRUE
    )

    install(FILES ${PROTO_OUT_PATH}/${PROTO_NAME}.pb.h
        DESTINATION include/${PROTO_DIR}
    )
ENDFUNCTION()


# Protobuf
set(PROTO_OUT_PATH "${CMAKE_BINARY_DIR}/proto-codegen")
proto_gen(${PROTO_OUT_PATH} supersonic/utils/exception/stack_trace.proto)
proto_gen(${PROTO_OUT_PATH} supersonic/utils/proto/expressions.proto)
proto_gen(${PROTO_OUT_PATH} supersonic/utils/proto/types.proto)
proto_gen(${PROTO_OUT_PATH} supersonic/benchmark/proto/benchmark.proto)
proto_gen(${PROTO_OUT_PATH} supersonic/cursor/proto/cursors.proto)
proto_gen(${PROTO_OUT_PATH} supersonic/expression/proto/operators.proto)
proto_gen(${PROTO_OUT_PATH} supersonic/proto/specification.proto)
proto_gen(${PROTO_OUT_PATH} supersonic/proto/supersonic.proto)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${PROTO_OUT_PATH}
)

# Main supersonic library
add_library(supersonic
    ${PROTO_OUT_PATH}/supersonic/utils/exception/stack_trace.pb.cc
    ${PROTO_OUT_PATH}/supersonic/utils/proto/expressions.pb.cc
    ${PROTO_OUT_PATH}/supersonic/utils/proto/types.pb.cc
    ${PROTO_OUT_PATH}/supersonic/benchmark/proto/benchmark.pb.cc
    ${PROTO_OUT_PATH}/supersonic/cursor/proto/cursors.pb.cc
    ${PROTO_OUT_PATH}/supersonic/expression/proto/operators.pb.cc
    ${PROTO_OUT_PATH}/supersonic/proto/specification.pb.cc
    ${PROTO_OUT_PATH}/supersonic/proto/supersonic.pb.cc

    supersonic/utils/bits.cc
    supersonic/utils/demangle.cc
    supersonic/utils/int128.cc
    supersonic/utils/mathlimits.cc
    supersonic/utils/strtoint.cc
    supersonic/utils/stringprintf.cc
    supersonic/utils/hash/hash.cc
    supersonic/utils/exception/stack_trace.cc
    supersonic/utils/strings/case.cc
    supersonic/utils/strings/join.cc
    supersonic/utils/strings/stringpiece.cc
    supersonic/utils/strings/ascii_ctype.cc
    supersonic/utils/strings/charset.cc
    supersonic/utils/strings/escaping.cc
    supersonic/utils/strings/human_readable.cc
    supersonic/utils/strings/memutil.cc
    supersonic/utils/strings/numbers.cc
    supersonic/utils/strings/serialize.cc
    supersonic/utils/strings/split.cc
    supersonic/utils/strings/strcat.cc
    supersonic/utils/strings/substitute.cc
    supersonic/utils/strings/strip.cc
    supersonic/utils/strings/util.cc
    supersonic/utils/utf/rune.c

    supersonic/utils/random.cc
    supersonic/utils/walltime.cc
    supersonic/utils/file.cc
    supersonic/utils/file_util.cc

    supersonic/base/infrastructure/bit_pointers.cc
    supersonic/base/infrastructure/block.cc
    supersonic/base/infrastructure/copy_column.cc
    supersonic/base/infrastructure/double_buffered_block.cc
    supersonic/base/infrastructure/projector.cc
    supersonic/base/infrastructure/tuple_schema.cc
    supersonic/base/infrastructure/types_infrastructure.cc
    supersonic/base/infrastructure/types.cc
    supersonic/base/infrastructure/view_copier.cc
    supersonic/base/memory/arena.cc
    supersonic/base/memory/memory.cc

    supersonic/cursor/base/cursor.cc
    supersonic/cursor/base/lookup_index.cc
    supersonic/cursor/core/aggregate_clusters.cc
    supersonic/cursor/core/aggregate_groups.cc
    supersonic/cursor/core/aggregate_scalar.cc
    supersonic/cursor/core/aggregator.cc
    supersonic/cursor/core/benchmarks.cc
    supersonic/cursor/core/coalesce.cc
    supersonic/cursor/core/column_aggregator.cc
    supersonic/cursor/core/compute.cc
    supersonic/cursor/core/filter.cc
    supersonic/cursor/core/foreign_filter.cc
    supersonic/cursor/core/generate.cc
    supersonic/cursor/core/hash_join.cc
    supersonic/cursor/core/hybrid_group_utils.cc
    supersonic/cursor/core/limit.cc
    supersonic/cursor/core/merge_union_all.cc
    supersonic/cursor/core/project.cc
    supersonic/cursor/core/rowid_merge_join.cc
    supersonic/cursor/core/scan_view.cc
    supersonic/cursor/core/sort.cc
    supersonic/cursor/core/specification_builder.cc
    supersonic/cursor/core/splitter.cc
    supersonic/cursor/core/spy.cc
    supersonic/cursor/infrastructure/basic_cursor.cc
    supersonic/cursor/infrastructure/basic_operation.cc
    supersonic/cursor/infrastructure/file_io.cc
    supersonic/cursor/infrastructure/iterators.cc
    supersonic/cursor/infrastructure/ordering.cc
    supersonic/cursor/infrastructure/row_hash_set.cc
    supersonic/cursor/infrastructure/table.cc
    supersonic/cursor/infrastructure/view_cursor.cc
    supersonic/cursor/infrastructure/view_printer.cc
    supersonic/cursor/infrastructure/writer.cc

    supersonic/expression/base/expression.cc
    supersonic/expression/core/arithmetic_bound_expressions.cc
    supersonic/expression/core/arithmetic_expressions.cc
    supersonic/expression/core/comparison_bound_expressions.cc
    supersonic/expression/core/comparison_expressions.cc
    supersonic/expression/core/date_bound_expressions.cc
    supersonic/expression/core/date_evaluators.cc
    supersonic/expression/core/date_expressions.cc
    supersonic/expression/core/elementary_bound_expressions.cc
    supersonic/expression/core/elementary_expressions.cc
    supersonic/expression/core/math_bound_expressions.cc
    supersonic/expression/core/math_expressions.cc
    supersonic/expression/core/projecting_bound_expressions.cc
    supersonic/expression/core/projecting_expressions.cc
    supersonic/expression/core/stateful_bound_expressions.cc
    supersonic/expression/core/stateful_expressions.cc
    supersonic/expression/core/string_bound_expressions.cc
    supersonic/expression/core/string_expressions.cc
    supersonic/expression/ext/hashing/hashing_bound_expressions.cc
    supersonic/expression/ext/hashing/hashing_expressions.cc
    supersonic/expression/infrastructure/basic_bound_expression.cc
    supersonic/expression/infrastructure/basic_expressions.cc
    supersonic/expression/infrastructure/expression_utils.cc
    supersonic/expression/infrastructure/terminal_bound_expressions.cc
    supersonic/expression/infrastructure/terminal_expressions.cc
    supersonic/expression/templated/bound_expression_factory.cc
    supersonic/expression/templated/cast_bound_expression.cc
    supersonic/expression/templated/cast_expression.cc
    supersonic/expression/vector/vector_logic.cc
)

set(HEADERS_SUPERSONIC
    supersonic/utils/algorithm.h
    supersonic/utils/arm_instruction_set_select.h
    supersonic/utils/basictypes.h
    supersonic/utils/bits.h
    supersonic/utils/charmap.h
    supersonic/utils/container_literal.h
    supersonic/utils/container_literal_generated.h
    supersonic/utils/container_logging.h
    supersonic/utils/cycleclock-inl.h
    supersonic/utils/demangle.h
    supersonic/utils/fixedarray.h
    supersonic/utils/integral_types.h
    supersonic/utils/iterator_adaptors.h
    supersonic/utils/endian.h
    supersonic/utils/casts.h
    supersonic/utils/int128.h
    supersonic/utils/linked_ptr.h
    supersonic/utils/logging-inl.h
    supersonic/utils/macros.h
    supersonic/utils/manual_constructor.h
    supersonic/utils/map_util.h
    supersonic/utils/mathlimits.h
    supersonic/utils/paranoid.h
    supersonic/utils/pointer_vector.h
    supersonic/utils/port.h
    supersonic/utils/sparsetable.h
    supersonic/utils/std_namespace.h
    supersonic/utils/strtoint.h
    supersonic/utils/stl_util.h
    supersonic/utils/stringprintf.h
    supersonic/utils/template_util.h
    supersonic/utils/valgrind.h
    supersonic/utils/hash/hash.h
    supersonic/utils/exception/coowned_pointer.h
    supersonic/utils/exception/failureor.h
    supersonic/utils/exception/stack_trace.h
    supersonic/utils/strings/ascii_ctype.h
    supersonic/utils/strings/case.h
    supersonic/utils/strings/charset.h
    supersonic/utils/strings/fastmem.h
    supersonic/utils/strings/join.h
    supersonic/utils/strings/join_internal.h
    supersonic/utils/strings/escaping.h
    supersonic/utils/strings/human_readable.h
    supersonic/utils/strings/memutil.h
    supersonic/utils/strings/numbers.h
    supersonic/utils/strings/serialize.h
    supersonic/utils/strings/split.h
    supersonic/utils/strings/split_internal.h
    supersonic/utils/strings/strcat.h
    supersonic/utils/strings/stringpiece.h
    supersonic/utils/strings/substitute.h
    supersonic/utils/strings/strip.h
    supersonic/utils/strings/util.h
    supersonic/utils/utf/utfdef.h
    supersonic/utils/timer.h
    supersonic/utils/random.h
    supersonic/utils/walltime.h
    supersonic/utils/file.h
    supersonic/utils/file_util.h
    supersonic/base/exception/exception_macros.h
    supersonic/base/exception/exception.h
    supersonic/base/exception/result.h
    supersonic/base/infrastructure/aggregation_operators.h
    supersonic/base/infrastructure/bit_pointers.h
    supersonic/base/infrastructure/block.h
    supersonic/base/infrastructure/copy_column.h
    supersonic/base/infrastructure/double_buffered_block.h
    supersonic/base/infrastructure/hasher.h
    supersonic/base/infrastructure/init.h
    supersonic/base/infrastructure/operators.h
    supersonic/base/infrastructure/projector.h
    supersonic/base/infrastructure/tuple_schema.h
    supersonic/base/infrastructure/types_infrastructure.h
    supersonic/base/infrastructure/types.h
    supersonic/base/infrastructure/variant_pointer.h
    supersonic/base/infrastructure/variant.h
    supersonic/base/infrastructure/view_copier.h
    supersonic/base/memory/arena.h
    supersonic/base/memory/memory.h
    supersonic/cursor/base/cursor.h
    supersonic/cursor/base/lookup_index.h
    supersonic/cursor/base/operation.h
    supersonic/cursor/base/cursor_transformer.h
    supersonic/cursor/core/aggregate.h
    supersonic/cursor/core/aggregator.h
    supersonic/cursor/core/benchmarks.h
    supersonic/cursor/core/coalesce.h
    supersonic/cursor/core/column_aggregator.h
    supersonic/cursor/core/compute.h
    supersonic/cursor/core/filter.h
    supersonic/cursor/core/foreign_filter.h
    supersonic/cursor/core/generate.h
    supersonic/cursor/core/hash_join.h
    supersonic/cursor/core/hybrid_group_utils.h
    supersonic/cursor/core/limit.h
    supersonic/cursor/core/merge_union_all.h
    supersonic/cursor/core/ownership_taker.h
    supersonic/cursor/core/project.h
    supersonic/cursor/core/rowid_merge_join.h
    supersonic/cursor/core/scan_view.h
    supersonic/cursor/core/sort.h
    supersonic/cursor/core/specification_builder.h
    supersonic/cursor/core/splitter.h
    supersonic/cursor/core/spy.h
    supersonic/cursor/infrastructure/basic_cursor.h
    supersonic/cursor/infrastructure/basic_operation.h
    supersonic/cursor/infrastructure/file_io.h
    supersonic/cursor/infrastructure/file_io-internal.h
    supersonic/cursor/infrastructure/history_transformer.h
    supersonic/cursor/infrastructure/iterators.h
    supersonic/cursor/infrastructure/ordering.h
    supersonic/cursor/infrastructure/ownership_revoker.h
    supersonic/cursor/infrastructure/row_copier.h
    supersonic/cursor/infrastructure/row.h
    supersonic/cursor/infrastructure/row_hash_set.h
    supersonic/cursor/infrastructure/table.h
    supersonic/cursor/infrastructure/value_ref.h
    supersonic/cursor/infrastructure/view_cursor.h
    supersonic/cursor/infrastructure/view_printer.h
    supersonic/expression/base/expression.h
    supersonic/expression/core/arithmetic_bound_expressions.h
    supersonic/expression/core/arithmetic_expressions.h
    supersonic/expression/core/comparison_bound_expressions.h
    supersonic/expression/core/comparison_expressions.h
    supersonic/expression/core/date_bound_expressions.h
    supersonic/expression/core/date_evaluators.h
    supersonic/expression/core/date_expressions.h
    supersonic/expression/core/elementary_bound_expressions.h
    supersonic/expression/core/elementary_expressions.h
    supersonic/expression/core/math_bound_expressions.h
    supersonic/expression/core/math_evaluators.h
    supersonic/expression/core/math_expressions.h
    supersonic/expression/core/projecting_bound_expressions.h
    supersonic/expression/core/projecting_expressions.h
    supersonic/expression/core/stateful_bound_expressions.h
    supersonic/expression/core/stateful_expressions.h
    supersonic/expression/core/string_bound_expressions.h
    supersonic/expression/core/string_evaluators.h
    supersonic/expression/core/string_expressions.h
    supersonic/expression/ext/hashing/hashing_bound_expressions.h
    supersonic/expression/ext/hashing/hashing_evaluators.h
    supersonic/expression/ext/hashing/hashing_expressions.h
    supersonic/expression/infrastructure/basic_bound_expression.h
    supersonic/expression/infrastructure/basic_expressions.h
    supersonic/expression/infrastructure/bound_expression_creators.h
    supersonic/expression/infrastructure/elementary_bound_const_expressions.h
    supersonic/expression/infrastructure/elementary_const_expressions.h
    supersonic/expression/infrastructure/expression_utils.h
    supersonic/expression/infrastructure/terminal_bound_expressions.h
    supersonic/expression/infrastructure/terminal_expressions.h
    supersonic/expression/templated/abstract_bound_expressions.h
    supersonic/expression/templated/abstract_expressions.h
    supersonic/expression/templated/bound_expression_factory.h
    supersonic/expression/templated/cast_bound_expression.h
    supersonic/expression/templated/cast_expression.h
    supersonic/expression/vector/binary_column_computers.h
    supersonic/expression/vector/column_validity_checkers.h
    supersonic/expression/vector/expression_evaluators.h
    supersonic/expression/vector/expression_traits.h
    supersonic/expression/vector/simd_operators.h
    supersonic/expression/vector/ternary_column_computers.h
    supersonic/expression/vector/unary_column_computers.h
    supersonic/expression/vector/vector_logic.h
    supersonic/expression/vector/vector_primitives.h
    supersonic/cursor/infrastructure/writer.h
    supersonic/utils/utf/utf.h
    supersonic/supersonic.h
)


target_compile_options(supersonic PUBLIC -Wno-ignored-attributes)

add_sanitizers(supersonic)

target_link_libraries(supersonic
    ${PROTOBUF_LIBRARIES}
    ${GFLAGS_LIBRARIES}
    ${GLOG_LIBRARIES}
    ${Boost_SYSTEM_LIBRARIES}
    ${Boost_TIMER_LIBRARIES}
    Threads::Threads
)

INSTALL_HEADERS_WITH_DIRECTORY(HEADERS_SUPERSONIC)
install(
    TARGETS supersonic
    DESTINATION "lib"
)


if(RE2_LIBRARY)
    add_definitions(-DHAVE_RE2_RE2_H)

    add_library(supersonic_regexp
        supersonic/expression/core/regexp_bound_expressions.cc
        supersonic/expression/core/regexp_expressions.cc
    )

    set(HEADERS_SUPERSONIC_RE2
        supersonic/expression/core/regexp_evaluators.h
        supersonic/expression/core/regexp_expressions.h
        supersonic/expression/core/regexp_bound_expressions.h
        supersonic/expression/core/regexp_bound_expressions_internal.h
    )


    add_sanitizers(supersonic_regexp)
    add_dependencies(supersonic_regexp supersonic)

    target_link_libraries(supersonic_regexp 
        supersonic
        ${RE2_LIBRARY}
    )

    INSTALL_HEADERS_WITH_DIRECTORY(HEADERS_SUPERSONIC_RE2)
    install(
        TARGETS supersonic_regexp
        DESTINATION "lib"
    )


endif(RE2_LIBRARY)


add_library(supersonic_serialization
    supersonic/serialization/build_expression_from_proto.cc
)


target_link_libraries(supersonic_serialization supersonic)
add_dependencies(supersonic_serialization supersonic)
add_sanitizers(supersonic_serialization)

INSTALL_HEADERS_WITH_DIRECTORY(supersonic/serialization/build_expression_from_proto.h)
install(
    TARGETS supersonic_serialization
    DESTINATION "lib")



enable_testing()

add_library(supersonic_testutils
    supersonic/testing/block_builder.cc
    supersonic/testing/repeating_block.cc
    supersonic/testing/streamable.cc
    supersonic/testing/view_comparator.cc
)

set(HEADERS_SUPERSONIC_TESTUTILS
    supersonic/testing/block_builder.h
    supersonic/testing/repeating_block.h
    supersonic/testing/streamable.h
    supersonic/testing/view_comparator.h
)

target_link_libraries(supersonic_testutils supersonic)
add_dependencies(supersonic_testutils supersonic)
add_sanitizers(supersonic_testutils)

install(
    TARGETS supersonic_testutils
    DESTINATION "lib"
)


add_library(supersonic_testutils_gunit
    supersonic/testing/comparable_cursor.cc
    supersonic/testing/comparable_tuple_schema.cc
    supersonic/testing/comparable_view.cc
    supersonic/testing/expression_test_helper.cc
    supersonic/testing/operation_testing.cc
    supersonic/testing/row.cc
    supersonic/testing/short_circuit_tester.cc
)
target_link_libraries(supersonic_testutils_gunit supersonic_testutils libgtest libgmock)
add_dependencies(supersonic_testutils_gunit supersonic_testutils libgtest libgmock)
add_sanitizers(supersonic_testutils_gunit)

INSTALL_HEADERS_WITH_DIRECTORY(HEADERS_SUPERSONIC_TESTUTILS)
install(
    TARGETS supersonic_testutils_gunit
    DESTINATION "lib"
)


set(TEST_LIBS supersonic_testutils_gunit libgtest libgtest_main libgmock)


# TEST:
add_executable(test_base_exception
    supersonic/base/exception/exception_test.cc
    supersonic/base/exception/result_test.cc
)

target_link_libraries(test_base_exception ${TEST_LIBS})
add_dependencies(test_base_exception supersonic_testutils)
add_sanitizers(test_base_exception)
add_test(base_exception test_base_exception)


# TEST:
add_executable(test_base_infrastructure
    supersonic/base/infrastructure/aggregation_operators_test.cc
    supersonic/base/infrastructure/bit_pointers_test.cc
    supersonic/base/infrastructure/block_test.cc
    supersonic/base/infrastructure/copy_column_test.cc
    supersonic/base/infrastructure/double_buffered_block_test.cc
    supersonic/base/infrastructure/operators_test.cc
    supersonic/base/infrastructure/projector_test.cc
    supersonic/base/infrastructure/types_infrastructure_test.cc
    supersonic/base/infrastructure/types_test.cc
    supersonic/base/infrastructure/variant_pointer_test.cc
    supersonic/base/infrastructure/view_copier_test.cc
)

target_link_libraries(test_base_infrastructure ${TEST_LIBS})
add_dependencies(test_base_infrastructure supersonic_testutils)
add_sanitizers(test_base_infrastructure)
add_test(base_infrastructure test_base_infrastructure)


# TEST:
add_executable(test_base_memory
    supersonic/base/memory/arena_test.cc
    supersonic/base/memory/memory_mocks.h
    supersonic/base/memory/memory_mocks.cc
    supersonic/base/memory/memory_test.cc
)

target_link_libraries(test_base_memory ${TEST_LIBS})
add_dependencies(test_base_memory supersonic_testutils)
add_sanitizers(test_base_memory)
add_test(base_memory test_base_memory)


# TEST: Rexexp expressions
if(RE2_LIBRARY)
    add_executable(test_regexp_expressions
        supersonic/expression/core/regexp_bound_expressions_test.cc
        supersonic/expression/core/regexp_expressions_test.cc
    )

    target_link_libraries(test_regexp_expressions supersonic_regexp ${TEST_LIBS})
    add_dependencies(test_regexp_expressions supersonic_regexp supersonic_testutils)
    add_sanitizers(test_regexp_expressions)
    add_test(regexp_expressions test_regexp_expressions)
endif(RE2_LIBRARY)


# TEST:
add_executable(test_cursor_base
    supersonic/cursor/base/lookup_index_test.cc
)

target_link_libraries(test_cursor_base ${TEST_LIBS})
add_dependencies(test_cursor_base supersonic_testutils)
add_sanitizers(test_cursor_base)
add_test(cursor_base test_cursor_base)


# TEST:
add_executable(test_cursor_core
    supersonic/cursor/core/aggregate_clusters_test.cc
    supersonic/cursor/core/aggregate_groups_test.cc
    supersonic/cursor/core/aggregate_scalar_test.cc
    supersonic/cursor/core/coalesce_test.cc
    supersonic/cursor/core/column_aggregator_test.cc
    supersonic/cursor/core/compute_test.cc
    supersonic/cursor/core/filter_test.cc
    supersonic/cursor/core/foreign_filter_test.cc
    supersonic/cursor/core/generate_test.cc
    supersonic/cursor/core/hash_join_test.cc
    supersonic/cursor/core/hybrid_aggregate_test.cc
    supersonic/cursor/core/hybrid_aggregate_large_test.cc
    supersonic/cursor/core/hybrid_group_utils_test.cc
    supersonic/cursor/core/limit_test.cc
    supersonic/cursor/core/merge_union_all_test.cc
    supersonic/cursor/core/project_test.cc
    supersonic/cursor/core/rowid_merge_join_test.cc
    supersonic/cursor/core/scan_view_test.cc
    supersonic/cursor/core/sort_test.cc
    supersonic/cursor/core/specification_builder_test.cc
    supersonic/cursor/core/splitter_test.cc
)

target_link_libraries(test_cursor_core ${TEST_LIBS})
add_dependencies(test_cursor_core supersonic_testutils)
add_sanitizers(test_cursor_core)
add_test(cursor_core test_cursor_core)


# TEST:
add_executable(test_cursor_infrastructure
    supersonic/cursor/infrastructure/basic_operation_test.cc
    supersonic/cursor/infrastructure/iterators_test.cc
    supersonic/cursor/infrastructure/row_copier_test.cc
    supersonic/cursor/infrastructure/row_hash_set_test.cc
    supersonic/cursor/infrastructure/row_test.cc
    supersonic/cursor/infrastructure/table_test.cc
    supersonic/cursor/infrastructure/view_cursor_test.cc
    supersonic/cursor/infrastructure/writer_test.cc
)

target_link_libraries(test_cursor_infrastructure ${TEST_LIBS})
add_dependencies(test_cursor_infrastructure supersonic_testutils)
add_sanitizers(test_cursor_infrastructure)
add_test(cursor_infrastructure test_cursor_infrastructure)



# TEST: Expressions
add_executable(test_expression_core
    supersonic/expression/core/arithmetic_bound_expressions_test.cc
    supersonic/expression/core/arithmetic_expressions_test.cc
    supersonic/expression/core/case_expression_test.cc
    supersonic/expression/core/comparison_bound_expressions_test.cc
    supersonic/expression/core/date_bound_expressions_test.cc
    supersonic/expression/core/date_expressions_test.cc
    supersonic/expression/core/elementary_bound_expressions_test.cc
    supersonic/expression/core/elementary_expressions_test.cc
    supersonic/expression/core/math_bound_expressions_test.cc
    supersonic/expression/core/math_expressions_test.cc
    supersonic/expression/core/projecting_bound_expressions_test.cc
    supersonic/expression/core/projecting_expressions_test.cc
    supersonic/expression/core/stateful_bound_expressions_test.cc
    supersonic/expression/core/stateful_expressions_test.cc
    supersonic/expression/core/string_bound_expressions_test.cc
    supersonic/expression/core/string_expressions_test.cc
)

target_link_libraries(test_expression_core ${TEST_LIBS})
add_dependencies(test_expression_core supersonic_testutils)
add_sanitizers(test_expression_core)
add_test(expression_core test_expression_core)


# TEST: 
add_executable(test_expression_infrastructure
    supersonic/expression/infrastructure/basic_bound_expression_test.cc
    supersonic/expression/infrastructure/expression_utils_test.cc
    supersonic/expression/infrastructure/terminal_expressions_test.cc
)

target_link_libraries(test_expression_infrastructure ${TEST_LIBS})
add_dependencies(test_expression_infrastructure supersonic_testutils)
add_sanitizers(test_expression_infrastructure)
add_test(expression_infrastructure test_expression_infrastructure)


# TEST: 
add_executable(test_expression_templated
    supersonic/expression/templated/cast_bound_expression_test.cc
    supersonic/expression/templated/cast_expression_test.cc
)

target_link_libraries(test_expression_templated ${TEST_LIBS})
add_dependencies(test_expression_templated supersonic_testutils)
add_sanitizers(test_expression_templated)
add_test(expression_templated test_expression_templated)


# TEST: SIMD expressions
add_executable(test_expression_vector
    supersonic/expression/vector/binary_column_computers_test.cc
    supersonic/expression/vector/column_validity_checkers_test.cc
    supersonic/expression/vector/ternary_column_computers_test.cc
    supersonic/expression/vector/unary_column_computers_test.cc
    supersonic/expression/vector/vector_logic_test.cc
    supersonic/expression/vector/vector_primitives_test.cc
)

target_link_libraries(test_expression_vector ${TEST_LIBS})
add_dependencies(test_expression_vector supersonic_testutils)
add_sanitizers(test_expression_vector)
add_test(expression_vector test_expression_vector)


# Benchmarking library
add_library(supersonic_benchmark
    supersonic/benchmark/dot/dot_drawer.cc
    supersonic/benchmark/examples/common_utils.cc
    supersonic/benchmark/infrastructure/benchmark_listener.cc
    supersonic/benchmark/infrastructure/benchmark_transformer.cc
    supersonic/benchmark/infrastructure/cursor_statistics.cc
    supersonic/benchmark/infrastructure/node.cc
    supersonic/benchmark/infrastructure/tree_builder.cc
    supersonic/benchmark/manager/benchmark_manager.cc
)

target_link_libraries(supersonic_benchmark supersonic_testutils)
add_dependencies(test_expression_vector supersonic_testutils)


# Sample benchmark
add_executable(operation_example
    supersonic/benchmark/examples/operation_example.cc
)

target_link_libraries(operation_example supersonic_benchmark)
add_dependencies(operation_example supersonic_benchmark)

