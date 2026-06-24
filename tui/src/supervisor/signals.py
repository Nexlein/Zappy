import signal
from contextlib import contextmanager


@contextmanager
def raise_on_signals(*signums: int):
    """Turn selected signals into ``SystemExit`` while the block runs."""

    def handler(signum, _frame):
        del _frame
        raise SystemExit(128 + signum)

    previous = {sig: signal.signal(sig, handler) for sig in signums}
    try:
        yield
    finally:
        for sig, prev in previous.items():
            signal.signal(sig, prev)
