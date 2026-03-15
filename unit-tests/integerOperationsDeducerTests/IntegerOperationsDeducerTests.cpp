#include <gtest/gtest.h>

#include <vector>
#include <map>
#include <string>

#include <nlohmann/json.hpp>

#include <compiler/IntegerOperationsDeducer.hpp>

nlohmann::json make_pragma(const std::vector<std::string>& symbols, int offset) {
    nlohmann::json j;
    j["offset"] = offset;
    for (const auto& s : symbols) {
        j["nodes"].push_back({{"identifier", s}});
    }
    return j;
}

TEST(IntegerOperationsDeducerShould, DetectIncOverflow) {
    std::vector<std::string> src = {"a", "b", "c"};
    std::vector<std::string> dst = {"a", "b", "c", "nan"};
    auto pragma = make_pragma({"a", "b", "c"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::string> map_inc = { {"a", "b"}, {"b", "c"}, {"c", "nan"} };
    auto res = deducer.getUnaryOperationForMap(src, dst, map_inc);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Overflow);
    EXPECT_EQ(res->operation, ArithmeticOperation::Inc);
}

TEST(IntegerOperationsDeducerShould, DetectDecOverflow) {
    std::vector<std::string> src = {"a", "b", "c"};
    std::vector<std::string> dst = {"a", "b", "c", "nan"};
    auto pragma = make_pragma({"a", "b", "c"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::string> map_dec = { {"a", "nan"}, {"b", "a"}, {"c", "b"} };
    auto res = deducer.getUnaryOperationForMap(src, dst, map_dec);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Overflow);
    EXPECT_EQ(res->operation, ArithmeticOperation::Dec);
}

TEST(IntegerOperationsDeducerShould, DetectIncModular) {
    std::vector<std::string> src = {"a", "b", "c"};
    std::vector<std::string> dst = {"a", "b", "c"};
    auto pragma = make_pragma({"a", "b", "c"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::string> map_inc = { {"a", "b"}, {"b", "c"}, {"c", "a"} };
    auto res = deducer.getUnaryOperationForMap(src, dst, map_inc);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Modular);
    EXPECT_EQ(res->operation, ArithmeticOperation::Inc);
}

TEST(IntegerOperationsDeducerShould, DetectDecModular) {
    std::vector<std::string> src = {"a", "b", "c"};
    std::vector<std::string> dst = {"a", "b", "c"};
    auto pragma = make_pragma({"a", "b", "c"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::string> map_dec = { {"a", "c"}, {"b", "a"}, {"c", "b"} };
    auto res = deducer.getUnaryOperationForMap(src, dst, map_dec);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Modular);
    EXPECT_EQ(res->operation, ArithmeticOperation::Dec);
}

TEST(IntegerOperationsDeducerShould, DetectIncSaturated) {
    std::vector<std::string> src = {"a", "b", "c"};
    std::vector<std::string> dst = {"a", "b", "c"};
    auto pragma = make_pragma({"a", "b", "c"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::string> map_inc = { {"a", "b"}, {"b", "c"}, {"c", "c"} };
    auto res = deducer.getUnaryOperationForMap(src, dst, map_inc);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Saturated);
    EXPECT_EQ(res->operation, ArithmeticOperation::Inc);
}

TEST(IntegerOperationsDeducerShould, DetectDecSaturated) {
    std::vector<std::string> src = {"a", "b", "c"};
    std::vector<std::string> dst = {"a", "b", "c"};
    auto pragma = make_pragma({"a", "b", "c"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::string> map_dec = { {"a", "a"}, {"b", "a"}, {"c", "b"} };
    auto res = deducer.getUnaryOperationForMap(src, dst, map_dec);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Saturated);
    EXPECT_EQ(res->operation, ArithmeticOperation::Dec);
}

TEST(IntegerOperationsDeducerShould, DetectAddOverflow) {
    std::vector<std::string> dom = {"a", "b", "c"};
    std::vector<std::string> dom_nan = {"a", "b", "c", "nan"};
    auto pragma = make_pragma({"a", "b", "c"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::map<std::string, std::string>> map_add = {
        {"a", { {"a", "a"}, {"b", "b"}, {"c", "c"} }},
        {"b", { {"a", "b"}, {"b", "c"}, {"c", "nan"} }},
        {"c", { {"a", "c"}, {"b", "nan"}, {"c", "nan"} }}
    };
    auto res = deducer.getBinaryOperationForMap(dom, dom, dom_nan, map_add);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Overflow);
    EXPECT_EQ(res->operation, ArithmeticOperation::Add);
}

TEST(IntegerOperationsDeducerShould, DetectSubOverflow) {
    std::vector<std::string> dom = {"a", "b", "c"};
    std::vector<std::string> dom_nan = {"a", "b", "c", "nan"};
    auto pragma = make_pragma({"a", "b", "c"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::map<std::string, std::string>> map_sub = {
        {"a", { {"a", "a"}, {"b", "nan"}, {"c", "nan"} }},
        {"b", { {"a", "b"}, {"b", "a"}, {"c", "nan"} }},
        {"c", { {"a", "c"}, {"b", "b"}, {"c", "a"} }}
    };
    auto res = deducer.getBinaryOperationForMap(dom, dom, dom_nan, map_sub);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Overflow);
    EXPECT_EQ(res->operation, ArithmeticOperation::Sub);
}

TEST(IntegerOperationsDeducerShould, DetectAddModular) {
    std::vector<std::string> dom = {"a", "b", "c"};
    auto pragma = make_pragma({"a", "b", "c"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::map<std::string, std::string>> map_add = {
        {"a", { {"a", "a"}, {"b", "b"}, {"c", "c"} }},
        {"b", { {"a", "b"}, {"b", "c"}, {"c", "a"} }},
        {"c", { {"a", "c"}, {"b", "a"}, {"c", "b"} }}
    };
    auto res = deducer.getBinaryOperationForMap(dom, dom, dom, map_add);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Modular);
    EXPECT_EQ(res->operation, ArithmeticOperation::Add);
}

TEST(IntegerOperationsDeducerShould, DetectSubModular) {
    std::vector<std::string> dom = {"a", "b", "c"};
    auto pragma = make_pragma({"a", "b", "c"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::map<std::string, std::string>> map_sub = {
        {"a", { {"a", "a"}, {"b", "c"}, {"c", "b"} }},
        {"b", { {"a", "b"}, {"b", "a"}, {"c", "c"} }},
        {"c", { {"a", "c"}, {"b", "b"}, {"c", "a"} }}
    };
    auto res = deducer.getBinaryOperationForMap(dom, dom, dom, map_sub);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Modular);
    EXPECT_EQ(res->operation, ArithmeticOperation::Sub);
}

TEST(IntegerOperationsDeducerShould, DetectAddSaturated) {
    std::vector<std::string> dom = {"a", "b", "c"};
    auto pragma = make_pragma({"a", "b", "c"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::map<std::string, std::string>> map_add = {
        {"a", { {"a", "a"}, {"b", "b"}, {"c", "c"} }},
        {"b", { {"a", "b"}, {"b", "c"}, {"c", "c"} }},
        {"c", { {"a", "c"}, {"b", "c"}, {"c", "c"} }}
    };
    auto res = deducer.getBinaryOperationForMap(dom, dom, dom, map_add);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Saturated);
    EXPECT_EQ(res->operation, ArithmeticOperation::Add);
}

TEST(IntegerOperationsDeducerShould, DetectSubSaturated) {
    std::vector<std::string> dom = {"a", "b", "c"};
    auto pragma = make_pragma({"a", "b", "c"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::map<std::string, std::string>> map_sub = {
        {"a", { {"a", "a"}, {"b", "a"}, {"c", "a"} }},
        {"b", { {"a", "b"}, {"b", "a"}, {"c", "a"} }},
        {"c", { {"a", "c"}, {"b", "b"}, {"c", "a"} }}
    };
    auto res = deducer.getBinaryOperationForMap(dom, dom, dom, map_sub);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Saturated);
    EXPECT_EQ(res->operation, ArithmeticOperation::Sub);
}

TEST(IntegerOperationsDeducerShould, DetectLess) {
    std::vector<std::string> dom = {"a", "b"};
    std::vector<std::string> resDom = {"f", "t"};
    auto pragma = make_pragma({"a", "b"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::map<std::string, std::string>> map_less = {
        {"a", { {"a", "f"}, {"b", "t"} }},
        {"b", { {"a", "f"}, {"b", "f"} }}
    };
    auto res = deducer.getBinaryOperationForMap(dom, dom, resDom, map_less);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Comparison);
    EXPECT_EQ(res->operation, ArithmeticOperation::Less);
}

TEST(IntegerOperationsDeducerShould, DetectGreater) {
    std::vector<std::string> dom = {"a", "b"};
    std::vector<std::string> resDom = {"f", "t"};
    auto pragma = make_pragma({"a", "b"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::map<std::string, std::string>> map_greater = {
        {"a", { {"a", "f"}, {"b", "f"} }},
        {"b", { {"a", "t"}, {"b", "f"} }}
    };
    auto res = deducer.getBinaryOperationForMap(dom, dom, resDom, map_greater);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Comparison);
    EXPECT_EQ(res->operation, ArithmeticOperation::Gr);
}

TEST(IntegerOperationsDeducerShould, DetectLessEqual) {
    std::vector<std::string> dom = {"a", "b"};
    std::vector<std::string> resDom = {"f", "t"};
    auto pragma = make_pragma({"a", "b"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::map<std::string, std::string>> map_less_equal = {
        {"a", { {"a", "t"}, {"b", "t"} }},
        {"b", { {"a", "f"}, {"b", "t"} }}
    };
    auto res = deducer.getBinaryOperationForMap(dom, dom, resDom, map_less_equal);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Comparison);
    EXPECT_EQ(res->operation, ArithmeticOperation::Leq);
}

TEST(IntegerOperationsDeducerShould, DetectGreaterEqual) {
    std::vector<std::string> dom = {"a", "b"};
    std::vector<std::string> resDom = {"f", "t"};
    auto pragma = make_pragma({"a", "b"}, 0);
    IntegerOperationsDeducer deducer;
    deducer.fillIntegerValuesInfo({pragma});
    std::map<std::string, std::map<std::string, std::string>> map_greater_equal = {
        {"a", { {"a", "t"}, {"b", "f"} }},
        {"b", { {"a", "t"}, {"b", "t"} }}
    };
    auto res = deducer.getBinaryOperationForMap(dom, dom, resDom, map_greater_equal);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->system, ArithmeticSystem::Comparison);
    EXPECT_EQ(res->operation, ArithmeticOperation::Ge);
}
