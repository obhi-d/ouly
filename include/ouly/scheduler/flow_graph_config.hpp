// SPDX-License-Identifier: MIT

#pragma once

namespace ouly::cfg
{

/**
 * @brief Pass the launching node's identifier to flow-graph tasks.
 *
 * When this option is present in a flow_graph or dynamic_flow_graph configuration, tasks may use
 * either the existing `(context)` signature or `(context, node_id)`. The latter identifies the node
 * whose firing launched the task.
 */
struct flow_graph_node_id
{
  static constexpr bool flow_graph_node_id_v = true;
};

} // namespace ouly::cfg

namespace ouly::detail
{

template <typename Config>
inline constexpr bool flow_graph_node_id_v = []
{
  if constexpr (requires { Config::flow_graph_node_id_v; })
  {
    return Config::flow_graph_node_id_v;
  }
  else
  {
    return false;
  }
}();

template <bool WithNodeId, typename Delegate, typename Context, typename NodeId>
struct flow_graph_delegate
{
  using type = Delegate;
};

template <typename Delegate, typename Context, typename NodeId>
struct flow_graph_delegate<true, Delegate, Context, NodeId>
{
  using type = typename Delegate::template rebind<void(Context const&, NodeId)>;
};

template <typename Config, typename Delegate, typename Context, typename NodeId>
using flow_graph_delegate_t =
 typename flow_graph_delegate<flow_graph_node_id_v<Config>, Delegate, Context, NodeId>::type;

} // namespace ouly::detail
