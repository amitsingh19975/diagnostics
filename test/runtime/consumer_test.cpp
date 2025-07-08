#include "diagnostics/basic.hpp"
#include "diagnostics/consumers/error_tracking.hpp"
#include "diagnostics/consumers/sorting.hpp"
#include "mock.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace dark;

TEST_CASE("Stream Consumer", "[stream_consumer]") {
    // TODO: Add tests after implementing consume function.
}

TEST_CASE("Error Tracking Consumer", "[error_consumer]") {
    auto stream_consumer = TestConsumer();
    {
        auto diag = Diagnostic{
            DiagnosticLevel::Error,
            core::BasicFormatter("TEst {}", 3),
            DiagnosticLocation {
                .filename = "tst.cpp",
                .source = {}
            }
        };

        auto consumer = ErrorTrackingDiagnosticConsumer(&stream_consumer);
        REQUIRE(consumer.seen_error() == false);

        consumer.consume(std::move(diag));
        REQUIRE(consumer.seen_error() == true);

        consumer.reset();
        REQUIRE(consumer.seen_error() == false);
    }

    {
        auto diag = Diagnostic{
            DiagnosticLevel::Warning,
            core::BasicFormatter("TEst {}", 3),
            DiagnosticLocation {
                .filename = "tst.cpp",
                .source = {}
            }
        };

        auto consumer = ErrorTrackingDiagnosticConsumer(&stream_consumer);
        REQUIRE(consumer.seen_error() == false);

        consumer.consume(std::move(diag));
        REQUIRE(consumer.seen_error() == false);

        consumer.reset();
        REQUIRE(consumer.seen_error() == false);
    }
}


TEST_CASE("Sorting Consumer", "[sorting_consumer]") {
    auto mock_consumer = TestConsumer();

    {
        auto diag1 = Diagnostic{
            .level = DiagnosticLevel::Error,
            .kind = DiagnosticKind::InvalidFunctionDefinition,
            .location = DiagnosticLocation {
                .filename = "b.cpp",
                .source = DiagnosticSourceLocationTokens::builder()
                    .begin_line(1, 0)
                        .add_token("void", 10, Span(10, 13))
                    .end_line()
                    .build()
            },
            .message = core::BasicFormatter("TEst {}", 3)
        };

        auto diag2 = Diagnostic{
            .level = DiagnosticLevel::Error,
            .kind = DiagnosticKind::InvalidFunctionDefinition,
            .location = DiagnosticLocation {
                .filename = "a.cpp",
                .source = DiagnosticSourceLocationTokens::builder()
                    .begin_line(1, 0)
                        .add_token("void", 10, Span(10, 13))
                    .end_line()
                    .build()
            },
            .message = core::BasicFormatter("TEst {}", 3)
        };

        auto consumer = SortingDiagnosticConsumer(&mock_consumer);
        consumer.consume(std::move(diag1));
        consumer.consume(std::move(diag2));

        REQUIRE(mock_consumer.diagnostics.empty() == true);

        consumer.flush();

        REQUIRE(mock_consumer.diagnostics.size() == 2);
        REQUIRE(mock_consumer.diagnostics[0].location.filename == "a.cpp");
        REQUIRE(mock_consumer.diagnostics[1].location.filename == "b.cpp");

        mock_consumer.clear();
    }

    {
        auto diag1 = Diagnostic{
            .level = DiagnosticLevel::Error,
            .kind = DiagnosticKind::InvalidFunctionDefinition,
            .location = DiagnosticLocation {
                .filename = "a.cpp",
                .source = DiagnosticSourceLocationTokens::builder()
                    .begin_line(2, 0)
                        .add_token("void", 10, Span(10, 13))
                    .end_line()
                    .build()
            },
            .message = core::BasicFormatter("TEst {}", 3)
        };

        auto diag2 = Diagnostic{
            .level = DiagnosticLevel::Error,
            .kind = DiagnosticKind::InvalidFunctionDefinition,
            .location = DiagnosticLocation {
                .filename = "a.cpp",
                .source = DiagnosticSourceLocationTokens::builder()
                    .begin_line(1, 0)
                        .add_token("void", 10, Span(10, 13))
                    .end_line()
                    .build()
            },
            .message = core::BasicFormatter("TEst {}", 3)
        };

        auto consumer = SortingDiagnosticConsumer(&mock_consumer);
        consumer.consume(std::move(diag1));
        consumer.consume(std::move(diag2));

        REQUIRE(mock_consumer.diagnostics.empty() == true);

        consumer.flush();

        REQUIRE(mock_consumer.diagnostics.size() == 2);
        REQUIRE(mock_consumer.diagnostics[0].location.line_info().first == 1);
        REQUIRE(mock_consumer.diagnostics[1].location.line_info().first == 2);

        mock_consumer.clear();
    }

    {
        auto diag1 = Diagnostic{
            .level = DiagnosticLevel::Error,
            .kind = DiagnosticKind::InvalidFunctionDefinition,
            .location = DiagnosticLocation {
                .filename = "a.cpp",
                .source = DiagnosticSourceLocationTokens::builder()
                    .begin_line(1, 0)
                        .add_token("void", 10, Span(10, 13))
                    .end_line()
                    .build()
            },
            .message = core::BasicFormatter("TEst {}", 3)
        };

        auto diag2 = Diagnostic{
            .level = DiagnosticLevel::Error,
            .kind = DiagnosticKind::InvalidFunctionDefinition,
            .location = DiagnosticLocation {
                .filename = "a.cpp",
                .source = DiagnosticSourceLocationTokens::builder()
                    .begin_line(1, 0)
                        .add_token("void", 5, Span(5, 8))
                    .end_line()
                    .build()
            },
            .message = core::BasicFormatter("TEst {}", 3)
        };


        auto consumer = SortingDiagnosticConsumer(&mock_consumer);
        consumer.consume(std::move(diag1));
        consumer.consume(std::move(diag2));

        REQUIRE(mock_consumer.diagnostics.empty() == true);

        consumer.flush();

        REQUIRE(mock_consumer.diagnostics.size() == 2);
        REQUIRE(mock_consumer.diagnostics[0].location.line_info().second == 6);
        REQUIRE(mock_consumer.diagnostics[1].location.line_info().second == 11);

        mock_consumer.clear();
    }
}
