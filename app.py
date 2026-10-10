import math
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

import streamlit as st

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / "build"
COMPILER = BUILD / "ptxjit"
GPU_RUNNER = BUILD / "gpu_runner"

st.set_page_config(
    page_title="PTX-JIT Studio",
    page_icon="⚡",
    layout="wide",
    initial_sidebar_state="expanded",
)

# ---------- Design system ----------
st.markdown("""
<style>
@import url('https://fonts.googleapis.com/css2?family=DM+Mono:wght@400;500&family=Manrope:wght@400;500;600;700;800&display=swap');

:root {
    --bg: #080b12;
    --panel: #101621;
    --panel2: #141c2a;
    --line: #263247;
    --muted: #94a3b8;
    --text: #edf3ff;
    --cyan: #5ce1e6;
    --blue: #7294ff;
    --green: #5de0a0;
}
html, body, [class*="css"] {
    font-family: 'Manrope', sans-serif;
}
.stApp {
    background:
      radial-gradient(ellipse at 75% 0%, rgba(58,92,180,.15), transparent 40%),
      radial-gradient(ellipse at 5% 25%, rgba(30,190,190,.07), transparent 32%),
      var(--bg);
    color: var(--text);
}
.block-container {
    max-width: 1450px;
    padding-top: 2rem;
    padding-bottom: 3rem;
}
[data-testid="stSidebar"] {
    background: #0c111b;
    border-right: 1px solid var(--line);
}
[data-testid="stSidebar"] > div:first-child {
    padding-top: 1.5rem;
}
section[data-testid="stSidebar"] p {
    color: #b8c4d8;
}
h1, h2, h3 {
    letter-spacing: -0.035em;
}
.hero {
    position: relative;
    overflow: hidden;
    padding: 30px 32px;
    border: 1px solid #2b3b53;
    border-radius: 22px;
    background: linear-gradient(120deg, #141e30 0%, #101621 56%, #12232d 100%);
    margin: 0 0 22px 0;
}
.hero:after {
    content: "";
    position: absolute;
    width: 250px;
    height: 250px;
    right: -80px;
    top: -120px;
    border-radius: 50%;
    border: 1px solid rgba(92,225,230,.25);
    box-shadow: 0 0 0 28px rgba(92,225,230,.025),
                0 0 0 58px rgba(92,225,230,.025);
}
.eyebrow {
    color: var(--cyan);
    font-size: 11px;
    font-weight: 800;
    letter-spacing: .18em;
    text-transform: uppercase;
    margin-bottom: 12px;
}
.hero-title {
    font-size: clamp(30px, 4vw, 46px);
    font-weight: 800;
    line-height: 1.12;
    letter-spacing: -.05em;
    color: #f3f7ff;
}
.hero-title span {
    background: linear-gradient(90deg, #62e8e3, #8ba7ff);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
}
.hero-sub {
    color: #aab8ce;
    margin-top: 12px;
    font-size: 14px;
    max-width: 680px;
    line-height: 1.8;
}
.pill {
    display: inline-block;
    border: 1px solid #31574f;
    border-radius: 30px;
    padding: 6px 11px;
    color: #86edbd;
    background: rgba(49,142,104,.10);
    font-size: 11px;
    font-weight: 700;
    margin-bottom: 15px;
}
.section-label {
    color: #a7b6cd;
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: .15em;
    font-weight: 800;
    margin: 22px 0 12px 0;
}
.metric-card {
    background: linear-gradient(145deg, #141d2b, #0e141f);
    border: 1px solid var(--line);
    border-radius: 15px;
    padding: 17px 18px;
    min-height: 108px;
}
.metric-label {
    color: #91a2bb;
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: .09em;
    font-weight: 700;
}
.metric-value {
    color: #edf5ff;
    font-size: 24px;
    font-weight: 800;
    margin-top: 8px;
    letter-spacing: -.04em;
}
.metric-note {
    color: #7e91ab;
    font-size: 11px;
    margin-top: 3px;
}
.stage {
    border: 1px solid var(--line);
    background: rgba(16,22,33,.85);
    border-radius: 12px;
    padding: 14px;
    min-height: 93px;
}
.stage-num {
    font-family: 'DM Mono', monospace;
    font-size: 10px;
    color: var(--cyan);
}
.stage-title {
    font-size: 13px;
    font-weight: 800;
    margin-top: 7px;
    color: #e6eefc;
}
.stage-note {
    font-size: 11px;
    color: #8192aa;
    margin-top: 4px;
}
.panel {
    background: linear-gradient(145deg, #121a27, #0e141e);
    border: 1px solid var(--line);
    border-radius: 17px;
    padding: 20px;
    margin-bottom: 15px;
}
.panel-title {
    color: #edf3ff;
    font-size: 16px;
    font-weight: 800;
    margin-bottom: 4px;
}
.panel-sub {
    color: #8d9eb6;
    font-size: 12px;
    margin-bottom: 15px;
}
div[data-testid="stTextInput"] input,
div[data-testid="stNumberInput"] input {
    background: #0b111b;
    border: 1px solid #2c3b51;
    border-radius: 10px;
}
div[data-testid="stTextInput"] input:focus,
div[data-testid="stNumberInput"] input:focus {
    border-color: var(--cyan);
    box-shadow: 0 0 0 1px var(--cyan);
}
.stButton > button[kind="primary"],
.stFormSubmitButton > button[kind="primary"] {
    background: linear-gradient(100deg, #4bd9d6, #8aa4ff);
    color: #08121b;
    font-weight: 800;
    border: 0;
    border-radius: 10px;
    min-height: 45px;
    transition: all .2s ease;
}
.stButton > button[kind="primary"]:hover,
.stFormSubmitButton > button[kind="primary"]:hover {
    filter: brightness(1.08);
    box-shadow: 0 0 22px rgba(92,225,230,.16);
}
div[data-testid="stTabs"] button {
    font-weight: 700;
}
div[data-testid="stCode"] {
    border: 1px solid #26354b;
    border-radius: 10px;
}
hr {
    border-color: #263247;
}
footer {visibility: hidden;}
#MainMenu {visibility: hidden;}
header[data-testid="stHeader"] {
    background: transparent;
}
</style>
""", unsafe_allow_html=True)


def run_command(args, timeout=30):
    return subprocess.run(
        [str(arg) for arg in args],
        cwd=ROOT,
        capture_output=True,
        text=True,
        timeout=timeout,
        check=False,
    )


def parse_number(output, label):
    match = re.search(
        rf"^{re.escape(label)}\s*:\s*([-+0-9.eEinfnaINFNA]+)",
        output,
        re.MULTILINE,
    )
    if match:
        try:
            return float(match.group(1))
        except ValueError:
            return None
    return None


def metric_card(label, value, note):
    st.markdown(
        f"""
        <div class="metric-card">
          <div class="metric-label">{label}</div>
          <div class="metric-value">{value}</div>
          <div class="metric-note">{note}</div>
        </div>
        """,
        unsafe_allow_html=True,
    )


# ---------- Sidebar ----------
with st.sidebar:
    st.markdown(
        """
        <div style="font-size:25px;font-weight:800;letter-spacing:-1px">
          ⚡ PTX<span style="color:#5ce1e6">JIT</span>
        </div>
        <div style="font-size:11px;color:#8395af;letter-spacing:2px;
                    margin-top:4px;margin-bottom:25px">
          COMPILER WORKSPACE
        </div>
        """,
        unsafe_allow_html=True,
    )

    st.markdown("### Runtime environment")
    st.markdown("**Language**  \n`C++17`")
    st.markdown("**Target architecture**  \n`sm_86`")
    st.markdown(
        f"**PTX assembler**  \n"
        f"{'🟢 Available' if shutil.which('ptxas') else '🟠 Not found'}"
    )
    st.markdown(
        f"**GPU runner**  \n"
        f"{'🟢 Built' if GPU_RUNNER.exists() else '🟠 Not built'}"
    )
    st.markdown(
        f"**Compiler binary**  \n"
        f"{'🟢 Ready' if COMPILER.exists() else '🔴 Missing'}"
    )
    st.divider()
    st.markdown("### Supported expressions")
    st.code("x * 2 + 3\n(x + 2) / 3\n-x * x", language="text")
    st.caption("Variable: x · Operators: + − × ÷ · Parentheses supported")
    st.divider()
    st.caption("LOCAL WORKSPACE · NVIDIA CUDA")


# ---------- Hero ----------
st.markdown(
    """
    <div class="hero">
      <div class="pill">● &nbsp; LOCAL COMPILER WORKSPACE</div>
      <div class="hero-title">Compile smarter.<br><span>Run on GPU.</span></div>
      <div class="hero-sub">
        Explore the compilation journey from a mathematical expression
        to optimized PTX assembly and real NVIDIA GPU execution.
        Inspect the output. Validate the result. Understand the pipeline.
      </div>
    </div>
    """,
    unsafe_allow_html=True,
)

# Pipeline visualization
st.markdown('<div class="section-label">Compilation pipeline</div>',
            unsafe_allow_html=True)
pipeline = st.columns(4)
stages = [
    ("01", "Expression", "Your input"),
    ("02", "C++ compiler", "AST + optimization"),
    ("03", "PTX assembler", "PTX → cubin"),
    ("04", "GPU runtime", "Execute + validate"),
]
for col, (number, title, note) in zip(pipeline, stages):
    with col:
        st.markdown(
            f"""
            <div class="stage">
              <div class="stage-num">STAGE {number}</div>
              <div class="stage-title">{title}</div>
              <div class="stage-note">{note}</div>
            </div>
            """,
            unsafe_allow_html=True,
        )

# ---------- Input panel ----------
st.markdown('<div class="section-label">Workspace / New compilation</div>',
            unsafe_allow_html=True)

with st.container(border=True):
    st.markdown("### Configure your expression")
    st.markdown(
        "<div style='color:#8d9eb6;font-size:13px;margin-bottom:14px'>"
        "Enter a supported scalar expression, choose a value for x, "
        "and launch the actual compiler.</div>",
        unsafe_allow_html=True,
    )

    with st.form("compile_form"):
        col_expr, col_input = st.columns([2.2, 1])
        with col_expr:
            expression = st.text_input(
                "Expression",
                value="x * 2 + 3",
                placeholder="e.g. (x + 2) * 3",
            )
        with col_input:
            input_value = st.number_input(
                "Input value · x",
                value=4.0,
                format="%.6f",
            )

        col_run, col_gpu, col_hint = st.columns([1.1, 1.35, 1.55])
        with col_run:
            submitted = st.form_submit_button(
                "⚡  Compile & run",
                type="primary",
                use_container_width=True,
            )
        with col_gpu:
            run_gpu = st.checkbox(
                "Execute on GPU",
                value=True,
            )
        with col_hint:
            st.markdown(
                "<div style='font-size:11px;color:#8d9eb6;padding-top:8px'>"
                "Local execution · No cloud API</div>",
                unsafe_allow_html=True,
            )

# ---------- Results ----------
if submitted:
    if not expression.strip():
        st.error("Enter an expression before compiling.")
        st.stop()

    if not COMPILER.exists():
        st.error(f"Compiler binary not found at {COMPILER}")
        st.code("cmake -S . -B build && cmake --build build -j2")
        st.stop()

    try:
        with st.spinner("Compiling expression with the C++ backend..."):
            with tempfile.TemporaryDirectory(prefix="ptxjit-") as temp:
                temp_dir = Path(temp)
                ptx_path = temp_dir / "kernel.ptx"
                cubin_path = temp_dir / "kernel.cubin"

                compiled = run_command(
                    [COMPILER, expression, ptx_path],
                    timeout=20,
                )

                if compiled.returncode != 0:
                    st.error("Compilation failed")
                    st.code(compiled.stderr or compiled.stdout)
                    st.stop()

                compiler_output = compiled.stdout
                ptx = ptx_path.read_text() if ptx_path.exists() else ""

                st.markdown(
                    '<div class="section-label">Compilation results</div>',
                    unsafe_allow_html=True,
                )

                # Extract actual AST node counts from compiler output.
                nodes = re.search(
                    r"AST nodes:\s*(\d+)\s*->\s*(\d+)",
                    compiler_output,
                )
                original_nodes = nodes.group(1) if nodes else "—"
                optimized_nodes = nodes.group(2) if nodes else "—"

                m1, m2, m3, m4 = st.columns(4)
                with m1:
                    metric_card("Compile status", "SUCCESS", "C++ compiler completed")
                with m2:
                    metric_card("AST nodes", original_nodes, "Before optimization")
                with m3:
                    metric_card("Optimized nodes", optimized_nodes, "After optimization")
                with m4:
                    metric_card("PTX size", f"{len(ptx.encode())} B", "Generated assembly")

                tab_ptx, tab_compile, tab_gpu, tab_help = st.tabs([
                    "⌘  PTX Assembly",
                    "⚙  Compiler Details",
                    "◉  GPU Execution",
                    "?  Expression Guide",
                ])

                with tab_ptx:
                    st.markdown("#### Generated PTX")
                    st.caption(
                        "Assembly emitted by your compiler. This is the actual "
                        "file passed to ptxas."
                    )
                    if ptx:
                        st.code(ptx, language="asm", line_numbers=True)
                        st.download_button(
                            "↓ Download PTX",
                            data=ptx,
                            file_name="ptxjit_kernel.ptx",
                            mime="text/plain",
                        )
                    else:
                        st.warning("The compiler did not produce a PTX file.")

                with tab_compile:
                    st.markdown("#### Compiler output")
                    st.code(compiler_output or "Compilation succeeded.")
                    st.markdown("#### Build environment")
                    st.code(
                        "Backend: C++17\n"
                        "Output: PTX assembly\n"
                        "Target: NVIDIA sm_86\n"
                        "Execution interface: CUDA Driver API"
                    )

                with tab_gpu:
                    st.markdown("#### GPU execution report")
                    if not run_gpu:
                        st.info("GPU execution was disabled for this run.")
                    elif not ptx:
                        st.warning("No PTX is available to assemble.")
                    elif not shutil.which("ptxas"):
                        st.warning("ptxas was not found. Install/configure CUDA tools.")
                    elif not GPU_RUNNER.exists():
                        st.warning("GPU runner binary is missing. Rebuild the project.")
                    else:
                        assembled = run_command(
                            ["ptxas", "-arch=sm_86", ptx_path, "-o", cubin_path],
                            timeout=30,
                        )
                        if assembled.returncode != 0:
                            st.error("PTX assembly failed")
                            st.code(assembled.stderr or assembled.stdout)
                        else:
                            st.success("PTX assembled successfully into a cubin.")
                            executed = run_command(
                                [GPU_RUNNER, cubin_path, expression, input_value],
                                timeout=30,
                            )
                            output = executed.stdout or executed.stderr
                            if executed.returncode != 0:
                                st.error("GPU execution failed")
                                st.code(output)
                            else:
                                gpu_value = parse_number(output, "GPU output")
                                cpu_value = parse_number(output, "CPU reference")
                                st.code(output)

                                if gpu_value is not None and cpu_value is not None:
                                    r1, r2, r3 = st.columns(3)
                                    with r1:
                                        metric_card("GPU result", f"{gpu_value:.8g}", "CUDA execution")
                                    with r2:
                                        metric_card("CPU reference", f"{cpu_value:.8g}", "CPU evaluator")
                                    error = abs(gpu_value - cpu_value)
                                    with r3:
                                        metric_card("Absolute error", f"{error:.3g}", "Numerical difference")

                                    if math.isclose(
                                        gpu_value,
                                        cpu_value,
                                        rel_tol=1e-5,
                                        abs_tol=1e-6,
                                    ):
                                        st.success("Validation passed · CPU and GPU results match.")
                                    else:
                                        st.error("Validation failed · CPU and GPU results differ.")
                                else:
                                    st.info(
                                        "Execution completed. See raw output above; "
                                        "the runner output format did not match the metric parser."
                                    )

                with tab_help:
                    st.markdown("#### Supported expression syntax")
                    st.code(
                        "x + 5\n"
                        "x * 2 + 3\n"
                        "(x + 2) * 3\n"
                        "x / 4 - 1\n"
                        "-x * x",
                        language="text",
                    )
                    st.markdown(
                        """
                        - Variable: `x`
                        - Floating-point constants
                        - Binary operators: `+`, `-`, `*`, `/`
                        - Unary plus and minus
                        - Parentheses for grouping

                        This is a scalar expression compiler, not a general
                        C++ or CUDA source compiler.
                        """
                    )

                st.caption(
                    "PTX and cubin artifacts are temporary for this run. "
                    "Download the PTX above if you want to keep it."
                )

    except subprocess.TimeoutExpired:
        st.error("A compiler or GPU command timed out.")
    except (OSError, UnicodeError) as exc:
        st.error(f"Could not run the local toolchain: {exc}")

else:
    st.markdown('<div class="section-label">Ready to compile</div>',
                unsafe_allow_html=True)
    st.markdown(
        """
        <div class="panel">
          <div class="panel-title">Your workspace is ready</div>
          <div class="panel-sub">
            Start with the example expression, or try one of the recipes below.
          </div>
          <div style="display:flex;gap:10px;flex-wrap:wrap">
            <span style="background:#172536;border:1px solid #30465e;
              border-radius:8px;padding:9px 12px;font-family:'DM Mono',monospace;
              color:#8de7e5;font-size:12px">x * 2 + 3</span>
            <span style="background:#172536;border:1px solid #30465e;
              border-radius:8px;padding:9px 12px;font-family:'DM Mono',monospace;
              color:#8de7e5;font-size:12px">(x + 2) * 3</span>
            <span style="background:#172536;border:1px solid #30465e;
              border-radius:8px;padding:9px 12px;font-family:'DM Mono',monospace;
              color:#8de7e5;font-size:12px">x * x - 4</span>
          </div>
        </div>
        """,
        unsafe_allow_html=True,
    )

st.markdown(
    """
    <div style="border-top:1px solid #263247;margin-top:32px;padding-top:16px;
                display:flex;justify-content:space-between;gap:12px;flex-wrap:wrap;
                color:#70819a;font-size:11px">
      <span>PTX-JIT STUDIO · GPU COMPILER WORKSPACE</span>
      <span>C++17 &nbsp;·&nbsp; PTX &nbsp;·&nbsp; CUDA DRIVER API</span>
    </div>
    """,
    unsafe_allow_html=True,
)
