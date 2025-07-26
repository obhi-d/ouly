# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'ouly'
copyright = '2025, obhi-d'
author = 'obhi-d'
release = '0.0.1'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
   "breathe"
]

breathe_projects = {
    "OULY": "../xml"
}
breathe_default_project = "OULY"
breathe_projects_source = {
    "allocators": ("../../include/ouly/allocators", [
        "allocator.hpp", "arena_allocator.hpp", "arena_options.hpp", "best_fit_options.hpp", 
        "coalescing_allocator.hpp", "coalescing_arena_allocator.hpp", "default_allocator.hpp", 
        "linear_allocator.hpp", "linear_arena_allocator.hpp", 
        "linear_stack_allocator.hpp", "memory_stats.hpp", "memory_tracker.hpp", "pool_allocator.hpp", 
        "std_allocator_wrapper.hpp", "std_short_alloc.hpp", "strat_best_fit_tree.hpp", 
        "strat_best_fit_v0.hpp", "strat_best_fit_v1.hpp", "strat_best_fit_v2.hpp", 
        "strat_greedy_v0.hpp", "strat_greedy_v1.hpp"
    ]),
    "containers": ("../../include/ouly/containers", [
        "array_types.hpp", "basic_queue.hpp", "blackboard.hpp", "index_map.hpp", "indirection.hpp", 
        "intrusive_list.hpp", "rbtree.hpp", "small_vector.hpp", "soavector.hpp", 
        "sparse_table.hpp", "sparse_vector.hpp", "table.hpp", "vlist.hpp"
    ]),
    "dsl": ("../../include/ouly/dsl", [
        "microexpr.hpp", "yaml.hpp"
    ]),
    "ecs": ("../../include/ouly/ecs", [
        "collection.hpp", "components.hpp", "entity.hpp", "registry.hpp"
    ]),
    "scheduler": ("../../include/ouly/scheduler", [
        "awaiters.hpp", "event_types.hpp", "parallel_for.hpp", "promise_type.hpp", "scheduler.hpp", 
        "spin_lock.hpp", "task.hpp", "worker.hpp", "task_context.hpp"
    ]),
    "serializers": ("../../include/ouly/serializers", [
        "binary_input_serializer.hpp", "binary_output_serializer.hpp", "binary_serializer.hpp", 
        "input_serializer.hpp", "output_serializer.hpp", "yaml_input_serializer.hpp", 
        "yaml_output_serializer.hpp"
    ]),
    "utils": ("../../include/ouly/utils", [
        "common.hpp", "config.hpp", "delegate.hpp", "error_codes.hpp", "external/komihash.h", 
        "external/wyhash.h", "external/wyhash32.h", "external/wyhash32.hpp", "integer_range.hpp", 
        "intrusive_ptr.hpp", "komihash.hpp", "nullable_optional.hpp", "program_args.hpp", 
        "projected_view.hpp", "reflection.hpp", "reflection_utils.hpp", "string_literal.hpp", 
        "string_utils.hpp", "tagged_int.hpp", "tagged_ptr.hpp", "tuple.hpp", "type_name.hpp", 
        "type_traits.hpp", "utils.hpp", "vector_abstraction.hpp", "word_list.hpp", "wyhash.hpp", 
        "zip_view.hpp"
    ])
}


templates_path = ['_templates']
exclude_patterns = []

language = 'C++'

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_rtd_theme"

# Theme options (optional, customize as needed)
html_theme_options = {
    "collapse_navigation": False,
    "navigation_depth": 4,
    "style_external_links": True,
    "titles_only": False
}
html_static_path = ['_static']
html_css_files = [
    "custom.css",
]
