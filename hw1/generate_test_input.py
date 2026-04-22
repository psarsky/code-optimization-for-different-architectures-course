from pathlib import Path


def make_block():
    ascii_part = (
        "HELLO\t\tWORLD\n"
        "This   IS    A   LONG TEST line... line!!!\n"
        "word word word unique unique unique next\n"
        "PUNCT: ! ? ; : . , - _ ( ) [ ] { } < > / \\ @ # $ % ^ & * + = ~ ` |\n"
        "MixED CaSe SENTENCE WITH DUPLICATE duplicate DUPLICATE words words words.\n"
        "spaces     tabs\t\tnewlines\n\n\nend\n"
        "accented: zalgolski zażółć gęślą jaźń\n"
    ).encode("utf-8")

    # include non-printable bytes (<32, >126)
    control = bytes([1, 2, 3, 7, 8, 11, 12, 14, 31, 127, 128, 129, 144, 200, 255])
    return ascii_part + control + b"\n"


def write_large_file(target, size_mb=32):
    block = make_block()
    target.parent.mkdir(parents=True, exist_ok=True)

    target_bytes = size_mb * 1024 * 1024
    with target.open("wb") as f:
        f.write(b"BEGIN_CASES\n")
        f.write(
            b"UPPER lower MiXeD\n"
            b"dup dup once once once\n"
            b"hello\t\t\tworld\n"
            b"line1\n\n\nline2\n"
            b"punct!!??..,,::;;\n"
            b"\x01\x02\x03\x7f\x80\xff\n"
            b"END_CASES\n"
        )
        written = f.tell()
        while written < target_bytes:
            f.write(block)
            written += len(block)


def write_ratio_file(target, blocks, noise_ratio):
    target.parent.mkdir(parents=True, exist_ok=True)
    base = (
        "alpha alpha beta beta gamma gamma delta delta\n"
        "One TWO three FOUR FIVE six SEVEN eight\n"
    ).encode("ascii")
    punct = b"!!! ??? ,,, ;;; ::: --- ___ (( )) [[ ]] {{ }}\n"
    whitespace = b"x\t\t\t y\nz\n\n\nq     r\n"
    control = bytes([1, 2, 3, 31, 127, 128, 255]) + b"\n"
    noisy = punct + whitespace + control
    clean = b"plain words with little change and no punctuation 1234\n"
    if len(clean) < len(noisy):
        clean = clean + b"a" * (len(noisy) - len(clean))
    else:
        clean = clean[: len(noisy)]

    with target.open("wb") as f:
        for i in range(blocks):
            f.write(base)
            if (i % 100) < int(noise_ratio * 100):
                f.write(noisy)
            else:
                f.write(clean)


if __name__ == "__main__":
    root = Path(__file__).resolve().parent
    write_large_file(root / "test_input_large.txt", size_mb=32)
    write_ratio_file(root / "test_input_low_mod.txt", blocks=120000, noise_ratio=0.15)
    write_ratio_file(root / "test_input_mid_mod.txt", blocks=120000, noise_ratio=0.50)
    write_ratio_file(root / "test_input_high_mod.txt", blocks=120000, noise_ratio=0.85)
