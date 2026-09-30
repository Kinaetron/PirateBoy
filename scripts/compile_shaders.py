import subprocess
import sys
from pathlib import Path

SOURCE_DIR = Path("shaders/source")
OUT_DIR = Path("shaders/compiled")

FORMATS = {
    "SPIRV": ".spv",
    "MSL": ".msl",
    "DXIL": ".dxil"
}

def main():
    for subfolder in FORMATS:
        (OUT_DIR / subfolder).mkdir(parents=True, exist_ok=True)

    shader_files = sorted(SOURCE_DIR.glob("*.hlsl"))

    if not shader_files:
        print(f"No .hlsl file found in {SOURCE_DIR}")
        return

    for shader_path in shader_files:
        name = shader_path.stem
        print(f"Compiling {name}...")

        for subfolder, extension in FORMATS.items():
            output_path = OUT_DIR / subfolder / f"{name}{extension}"

            result = subprocess.run(
                ["shadercross", str(shader_path), "-o", str(output_path)],
                capture_output=True,
                text=True
            )

            if result.returncode != 0:
                print(f"Failed to compile {name} -> {output_path}", file=sys.stderr)
                print(result.stderr, file=sys.stderr)
                sys.exit(1)

    print("Done.")

if __name__ == "__main__":
    main()