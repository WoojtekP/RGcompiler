#include <common/GraphCreator.hpp>
#include <compiler/SymbolsManager.hpp>
#include <graph/Action.hpp>
#include <graph/ExpressionFactory.hpp>
#include <parser/Parser.hpp>

namespace GraphCreator
{
std::unique_ptr<Node> createUniqueNode(const std::string& nodeName)
{
    nlohmann::json label = {{"kind", "Literal"}, {"identifier", nodeName}};

    return std::make_unique<Node>(label);
}

std::shared_ptr<Node> createNode(const std::string& nodeName)
{
    nlohmann::json label = {{"kind", "Literal"}, {"identifier", nodeName}};

    return std::make_shared<Node>(label);
}

std::shared_ptr<IAction> createTagAction(const std::string& tag)
{
    nlohmann::json label = {{"kind", "Tag"}, {"symbol", tag}};

    return std::make_shared<ActionTag>(label);
}

std::shared_ptr<IAction> createAssignmentAction(const std::string& leftSide, const std::string& rightSide)
{
    nlohmann::json parsedJson = nlohmann::json::parse(R"({"types": {}, "variables": {}, "constants": {}})");
    Parser parser(parsedJson);
    SymbolsManager symbolsManager(parser);
    ExpressionFactory expressionFactory(parser, symbolsManager);
    nlohmann::json label = {
        {"lhs", {{"identifier", leftSide}, {"kind", "Reference"}}},
        {"rhs",
         {{"kind", "Cast"},
          {"lhs", {{"kind", "TypeReference"}, {"identifier", ""}}},
          {"rhs", {{"kind", "Reference"}, {"identifier", rightSide}}}}}};

    return std::make_shared<ActionAssignment>(label, expressionFactory);
}

std::shared_ptr<IAction> createReachabilityAction(const std::string& leftSide, const std::string& rightSide)
{
    nlohmann::json parsedJson = nlohmann::json::parse(R"({"types": {}, "variables": {}, "constants": {}})");
    Parser parser(parsedJson);
    SymbolsManager symbolsManager(parser);
    ExpressionFactory expressionFactory(parser, symbolsManager);
    nlohmann::json label = {
        {"kind", "Reachability"},
        {"lhs", {{"kind", "Node"}, {"identifier", leftSide}}},
        {"rhs", {{"kind", "Node"}, {"identifier", rightSide}}},
        {"negated", false}};

    return std::make_shared<ActionReachability>(label, expressionFactory);
}

void addEdge(
    std::shared_ptr<Graph>& graph,
    const std::string& fromNodeName,
    const std::string& toNodeName,
    const std::shared_ptr<IAction>& action)
{
    graph->addEdge(std::make_shared<Edge>(
        createNode(fromNodeName), createNode(toNodeName), std::vector<std::shared_ptr<IAction>> {action}));
}
}  // namespace GraphCreator
