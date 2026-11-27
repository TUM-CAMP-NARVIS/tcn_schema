import tempfile
import unittest
from pathlib import Path

from check_links import find_broken_links


class CheckLinksTest(unittest.TestCase):
    def test_reports_missing_target(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "a.md").write_text("[x](b.md)\n")
            self.assertEqual(find_broken_links(root), [(root / "a.md", 1, "b.md")])

    def test_ignores_urls_and_anchors(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "a.md").write_text("[x](https://e.org)\n[y](#top)\n[z](a.md#sec)\n")
            self.assertEqual(find_broken_links(root), [])

    def test_ignores_links_in_code(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "a.md").write_text("Write `[x](missing.md)` like this.\n```\n[y](gone.md)\n```\n")
            self.assertEqual(find_broken_links(root), [])

    def test_resolves_relative_to_the_file(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "sub").mkdir()
            (root / "b.md").write_text("")
            (root / "sub" / "a.md").write_text("[up](../b.md)\n[bad](b.md)\n")
            self.assertEqual(find_broken_links(root), [(root / "sub" / "a.md", 2, "b.md")])


if __name__ == "__main__":
    unittest.main()
