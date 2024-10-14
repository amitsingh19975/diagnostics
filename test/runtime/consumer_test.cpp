#include "diagnostics/basic.hpp"
#include "mock.hpp"
#include <catch2/catch_test_macros.hpp>
#include <sstream>

using namespace dark;

TEST_CASE("Stream Consumer", "[stream_consumer]") {
    // TODO: Add tests after implementing consume function.
}

TEST_CASE("Error Tracking Consumer", "[error_consumer]") {
    auto ss = std::stringstream{};
    auto stream = Stream(ss);
    auto stream_consumer = BasicStreamDiagnosticConsumer<>(stream);
    {
        auto diag = BasicDiagnostic<>{
            DiagnosticLevel::Error,
            core::Formatter("TEst {}", 3),
            DiagnosticLocation {
                .filename = "tst.cpp",
                .source = BasicDiagnosticLocationItem {}
            }
        };

        auto consumer = BasicErrorTrackingDiagnosticConsumer<>(&stream_consumer);
        REQUIRE(consumer.seen_error() == false);

        consumer.consume(std::move(diag));
        REQUIRE(consumer.seen_error() == true);

        consumer.reset();
        REQUIRE(consumer.seen_error() == false);
    }

    {
        auto diag = BasicDiagnostic<>{
            DiagnosticLevel::Warning,
            core::Formatter("TEst {}", 3),
            DiagnosticLocation {
                .filename = "tst.cpp",
                .source = BasicDiagnosticLocationItem {}
            }
        };

        auto consumer = BasicErrorTrackingDiagnosticConsumer<>(&stream_consumer);
        REQUIRE(consumer.seen_error() == false);

        consumer.consume(std::move(diag));
        REQUIRE(consumer.seen_error() == false);

        consumer.reset();
        REQUIRE(consumer.seen_error() == false);
    }
}


TEST_CASE("Sorting Consumer", "[sorting_consumer]") {
    auto mock_consumer = MockConsumer();

    {
        auto diag1 = BasicDiagnostic<DiagnosticKind>{
            DiagnosticKind::InvalidFunctionDefinition,
            DiagnosticLevel::Error,
            core::Formatter("TEst {}", 3),
            DiagnosticLocation {
                .filename = "b.cpp",
                .source = BasicDiagnosticLocationItem {
                    .source = "",
                    .line_number = 1,
                    .column_number = 2
                }
            }
        };

        auto diag2 = BasicDiagnostic<DiagnosticKind>{
            DiagnosticKind::InvalidFunctionDefinition,
            DiagnosticLevel::Error,
            core::Formatter("TEst {}", 3),
            DiagnosticLocation {
                .filename = "a.cpp",
                .source = BasicDiagnosticLocationItem {
                    .source = "",
                    .line_number = 1,
                    .column_number = 2
                }
            }
        };

        auto consumer = BasicSortingDiagnosticConsumer<DiagnosticKind>(&mock_consumer);
        consumer.consume(std::move(diag1));
        consumer.consume(std::move(diag2));

        REQUIRE(mock_consumer.diags.empty() == true);

        consumer.flush();

        REQUIRE(mock_consumer.diags.size() == 2);
        REQUIRE(mock_consumer.diags[0].location.filename == "a.cpp");
        REQUIRE(mock_consumer.diags[1].location.filename == "b.cpp");

        mock_consumer.clear();
    }

    {
        auto diag1 = BasicDiagnostic<DiagnosticKind>{
            DiagnosticKind::InvalidFunctionDefinition,
            DiagnosticLevel::Error,
            core::Formatter("TEst {}", 3),
            DiagnosticLocation {
                .filename = "a.cpp",
                .source = BasicDiagnosticLocationItem {
                    .source = "",
                    .line_number = 2,
                    .column_number = 2
                }
            }
        };

        auto diag2 = BasicDiagnostic<DiagnosticKind>{
            DiagnosticKind::InvalidFunctionDefinition,
            DiagnosticLevel::Error,
            core::Formatter("TEst {}", 3),
            DiagnosticLocation {
                .filename = "a.cpp",
                .source = BasicDiagnosticLocationItem {
                    .source = "",
                    .line_number = 1,
                    .column_number = 2
                }
            }
        };

        auto consumer = BasicSortingDiagnosticConsumer<DiagnosticKind>(&mock_consumer);
        consumer.consume(std::move(diag1));
        consumer.consume(std::move(diag2));

        REQUIRE(mock_consumer.diags.empty() == true);

        consumer.flush();

        REQUIRE(mock_consumer.diags.size() == 2);
        REQUIRE(mock_consumer.diags[0].location.get_as_location_item().line_number == 1);
        REQUIRE(mock_consumer.diags[1].location.get_as_location_item().line_number == 2);

        mock_consumer.clear();
    }

    {
        auto diag1 = BasicDiagnostic<DiagnosticKind>{
            DiagnosticKind::InvalidFunctionDefinition,
            DiagnosticLevel::Error,
            core::Formatter("TEst {}", 3),
            DiagnosticLocation {
                .filename = "a.cpp",
                .source = BasicDiagnosticLocationItem {
                    .source = "",
                    .line_number = 1,
                    .column_number = 4
                }
            }
        };

        auto diag2 = BasicDiagnostic<DiagnosticKind>{
            DiagnosticKind::InvalidFunctionDefinition,
            DiagnosticLevel::Error,
            core::Formatter("TEst {}", 3),
            DiagnosticLocation {
                .filename = "a.cpp",
                .source = BasicDiagnosticLocationItem {
                    .source = "",
                    .line_number = 1,
                    .column_number = 2
                }
            }
        };

        auto consumer = BasicSortingDiagnosticConsumer<DiagnosticKind>(&mock_consumer);
        consumer.consume(std::move(diag1));
        consumer.consume(std::move(diag2));

        REQUIRE(mock_consumer.diags.empty() == true);

        consumer.flush();

        REQUIRE(mock_consumer.diags.size() == 2);
        REQUIRE(mock_consumer.diags[0].location.get_as_location_item().column_number == 2);
        REQUIRE(mock_consumer.diags[1].location.get_as_location_item().column_number == 4);

        mock_consumer.clear();
    }
}
