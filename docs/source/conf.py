# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import re
import shutil
import subprocess
from pathlib import Path

_docs_source = Path(__file__).resolve().parent
_repo_root = _docs_source.parent.parent

project = 'OULY'
copyright = '2025, OULY Contributors'
author = 'OULY Contributors'


def _project_version() -> str:
    """The version from `project(ouly VERSION x.y.z)` in CMakeLists.txt."""
    try:
        text = (_repo_root / 'CMakeLists.txt').read_text(encoding='utf-8')
    except OSError:
        return 'unknown'
    match = re.search(r'project\s*\(\s*ouly\s+VERSION\s+([0-9.]+)', text)
    return match.group(1) if match else 'unknown'


release = _project_version()
version = release


def _ensure_doxygen_xml() -> None:
    """Generate docs/xml for Breathe when it is missing (e.g. on Read the Docs).

    Equivalent to `doxygen Doxyfile` from the repository root, but only writes the XML output.
    """
    if (_repo_root / 'docs' / 'xml' / 'index.xml').exists():
        return
    if shutil.which('doxygen') is None:
        raise RuntimeError('docs/xml is missing and doxygen is not installed: install doxygen, or run '
                           '`doxygen Doxyfile` from the repository root before sphinx-build')
    doxyfile = (_repo_root / 'Doxyfile').read_text(encoding='utf-8')
    # Doxygen reports every undocumented member; keep that out of the Sphinx log
    subprocess.run(['doxygen', '-'], input=doxyfile + '\nGENERATE_HTML=NO\nGENERATE_LATEX=NO\n',
                   text=True, cwd=_repo_root, check=False, stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL)


_ensure_doxygen_xml()

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'sphinx.ext.napoleon',
    'sphinx.ext.viewcode',
    'sphinx.ext.todo',
    'breathe',
    'sphinx.ext.graphviz',
    'sphinx.ext.imgmath',
]

templates_path = ['_templates']
exclude_patterns = []

# -- Breathe configuration --------------------------------------------------
breathe_projects = {"OULY": "../xml/"}
breathe_default_project = "OULY"
breathe_default_members = ('members', 'undoc-members')

# -- Options for HTML output ------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

# Theme options
html_theme_options = {
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    'vcs_pageview_mode': '',
    # Toc options
    'collapse_navigation': True,
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False
}

# -- Napoleon settings -------------------------------------------------------
napoleon_google_docstring = True
napoleon_numpy_docstring = True
napoleon_include_init_with_doc = False
napoleon_include_private_with_doc = False
napoleon_include_special_with_doc = True
napoleon_use_admonition_for_examples = False
napoleon_use_admonition_for_notes = False
napoleon_use_admonition_for_references = False
napoleon_use_ivar = False
napoleon_use_param = True
napoleon_use_rtype = True
napoleon_preprocess_types = False
napoleon_type_aliases = None
napoleon_attr_annotations = True

# -- Options for todo extension ----------------------------------------------
todo_include_todos = True

# -- Custom CSS --------------------------------------------------------------
def setup(app):
    app.add_css_file('custom.css')
