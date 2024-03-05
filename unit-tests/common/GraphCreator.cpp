#include <common/GraphCreator.hpp>
#include <graph/Action.hpp>

namespace GraphCreator
{
std::shared_ptr<Node> createNode(const std::string& nodeName)
{
    nlohmann::json label = {{{"kind", "Literal"}, {"identifier", nodeName}}};

    return std::make_shared<Node>(label);
}

std::shared_ptr<IAction> createAssignmentAction(const std::string& leftSide, const std::string& rightSide)
{
    // std::ifstream jsonEmptyFile;
    // Temporary solution that may end up in crash, as there was no time to pepare proper initializig of Parser
    Parser* parser = nullptr;
    ValueAssigner valueAssigner(nlohmann::json(
        {{{"kind", "TypeDeclaration"},
          {"identifier", "Player"},
          {"type", {{"kind", "Set"}, {"identifiers", {"white", "black"}}}}}}));
    ExpressionFactory expressionFactory(*parser, valueAssigner);
    nlohmann::json label = {
        {"lhs",
         {{"kind", "Cast"},
          {"lhs", {{"kind", "TypeReference"}, {"identifier", ""}}},
          {"rhs", {{"kind", "Reference"}, {"identifier", leftSide}}}}},
        {"rhs",
         {{"kind", "Cast"},
          {"lhs", {{"kind", "TypeReference"}, {"identifier", ""}}},
          {"rhs", {{"kind", "Reference"}, {"identifier", rightSide}}}}}};

    return std::make_shared<ActionAssignment>(label, expressionFactory);
}

std::shared_ptr<IAction> createReachabilityAction(const std::string& leftSide, const std::string& rightSide)
{
    Parser* parser = nullptr;
    ValueAssigner valueAssigner(nlohmann::json(
        {{{"kind", "TypeDeclaration"},
          {"identifier", "Player"},
          {"type", {{"kind", "Set"}, {"identifiers", {"white", "black"}}}}}}));
    ExpressionFactory expressionFactory(*parser, valueAssigner);
    nlohmann::json label = {
        {"kind", "Reachability"},
        {"lhs", {{"kind", "EdgeName"}, {"parts", {{{"kind", "Literal"}, {"identifier", leftSide}}}}}},
        {"rhs", {{"kind", "EdgeName"}, {"parts", {{{"kind", "Literal"}, {"identifier", rightSide}}}}}},
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