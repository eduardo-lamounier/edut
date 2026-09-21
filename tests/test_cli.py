"""CLI regression tests: python3 tests/test_cli.py build/edut (Unix)."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


EXECUTABLE = Path(sys.argv.pop(1) if len(sys.argv) > 1 else "build/edut").resolve()

CONFIG = r'''
local api = require "edut"
local function inspect(input)
  for _, index in ipairs({-1, 0, 11, 4294967297, math.maxinteger or 9007199254740991}) do
    assert(input.get_argument(index) == nil)
  end
  for index = 1, 10 do
    local value = input.get_argument(index)
    if value then print("arg:" .. index .. ":" .. value) end
  end
  for _, name in ipairs({"--output-file=", "--pair", "--many"}) do
    if input.contains_flag(name) then
      for index = 1, 10 do
        local value = input.get_argument(name, index)
        if value then print(name .. ":" .. index .. ":" .. value) end
      end
    end
  end
  local sub = input.get_subcommand()
  if sub then sub.execute(input.for_subcommand()) end
  print("EXECUTED")
end
api.setup { commands = {
  { "plain", execute = inspect },
  { "test", flags = {
      "--verbose", "-v", ["--output-file="] = 1, ["--pair"] = 2, ["--many"] = 10,
    }, subcommands = {{ "child", execute = inspect }}, execute = inspect },
} }
'''


class CliTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.config = self.root / "edut" / "init.lua"
        self.config.parent.mkdir()
        self.config.write_text(CONFIG)
        self.env = dict(os.environ, XDG_CONFIG_HOME=str(self.root))

    def run_cli(self, *args, success=True):
        result = subprocess.run(
            [str(EXECUTABLE), *args], env=self.env, cwd=self.root,
            capture_output=True, text=True, timeout=10,
        )
        output = result.stdout + result.stderr
        if success:
            self.assertEqual(result.returncode, 0, output)
            self.assertIn("EXECUTED", output)
            self.assertNotIn("ERROR", output)
        else:
            self.assertNotEqual(result.returncode, 0, output)
            self.assertNotIn("EXECUTED", output)
        return output

    def test_home_fallback(self):
        home = self.root / "home"
        target = home / ".config" / "edut"
        target.mkdir(parents=True)
        (target / "init.lua").write_text(CONFIG)
        self.env["HOME"] = str(home)
        for value in (None, ""):
            with self.subTest(xdg=value):
                self.env.pop("XDG_CONFIG_HOME", None)
                if value is not None:
                    self.env["XDG_CONFIG_HOME"] = value
                self.run_cli("plain")

    def test_no_flags_and_positional_limits(self):
        self.run_cli("plain")
        self.run_cli("plain", "value")
        self.run_cli("test", "child", "value")
        values = [str(i) for i in range(10)]
        self.assertIn("arg:10:9", self.run_cli("plain", *values))
        self.run_cli("plain", *values, "overflow", success=False)
        self.run_cli("test", "child", *values, "overflow", success=False)

    def test_attached_and_separate_values(self):
        for args in (("--output-file=", "a b.txt"), ("--output-file=a b.txt",)):
            self.assertIn("--output-file=:1:a b.txt", self.run_cli("test", *args))
        self.assertIn("--pair:2:second", self.run_cli("test", "--pair=first", "second"))
        self.assertIn("--output-file=:1:a=b", self.run_cli("test", "--output-file=a=b"))
        self.assertIn("--pair:1:\n", self.run_cli("test", "--pair=", "second"))
        long_value = "x" * 100
        self.assertIn(long_value, self.run_cli("test", "--output-file=" + long_value))
        self.run_cli("test", "--verbose=yes", success=False)
        self.run_cli("test", "--unknown=value", success=False)

    def test_required_values(self):
        for args in (
            ("--output-file=",), ("--pair", "first"), ("--pair=first",),
            ("--output-file=", "--verbose"), ("--output-file=", "-v"),
            ("--output-file=", "child"), ("--output-file=", "--unknown"),
        ):
            with self.subTest(args=args):
                self.run_cli("test", *args, success=False)
        self.run_cli("test", "--output-file=child", "child")
        self.run_cli("test", "--pair", "-1", "-2")
        self.run_cli("test", "--many", *map(str, range(10)))
        self.run_cli("test", "--verbose", "--output-file=", "file", "child")

    def test_repeated_flag_limits(self):
        self.run_cli("test", *(["--verbose"] * 20))
        self.run_cli("test", *(["--verbose"] * 21), success=False)

    def test_registration_limits(self):
        for count in (20, 21):
            self.config.write_text('''
local flags = {}
for i = 1, %d do flags[i] = "--flag" .. i end
require "edut".setup {commands = {{"limit", flags = flags,
  execute = function() print("EXECUTED") end}}}
''' % count)
            output = self.run_cli("limit", "--flag20", success=count == 20)
            if count == 21:
                self.assertIn("Too many flags", output)

    def test_invalid_flag_arity(self):
        for count in (-1, 11, 1.5):
            self.config.write_text('''
require "edut".setup {commands = {{"limit", flags = {["--bad"] = %s},
  execute = function() print("EXECUTED") end}}}
''' % count)
            self.run_cli("limit", "--bad", success=False)

    def test_ambiguous_flag_names(self):
        for flags in (
            '{["--out"] = 1, ["--out="] = 1}',
            '{"--out", ["--out="] = 1}',
            '{"--out", "--out"}',
        ):
            with self.subTest(flags=flags):
                self.config.write_text('''
require "edut".setup {commands = {{"test", flags = %s,
  execute = function() print("EXECUTED") end}}}
''' % flags)
                output = self.run_cli("test", "--out=value", success=False)
                self.assertIn("Ambiguous flag names", output)

    def test_caught_setup_error_preserves_previous_commands(self):
        for invalid in (
            '{"broken", flags = {["--bad"] = -1}, execute = function() end}',
            '{"broken", subcommands = {{"child", flags = {["--bad"] = -1}, '
            'execute = function() end}}, execute = function() end}',
        ):
            with self.subTest(invalid=invalid):
                self.config.write_text(CONFIG + '''
local ok = pcall(require "edut".setup, {commands = {
  {"partial", execute = function() print("EXECUTED") end},
  %s,
  {"later", execute = function() print("EXECUTED") end},
}})
assert(not ok)
''' % invalid)
                self.run_cli("plain", "still-registered")
                self.run_cli("test", "child")
                self.run_cli("partial", success=False)
                self.run_cli("later", success=False)

    def test_caught_first_setup_error_leaves_no_commands(self):
        self.config.write_text('''
local ok = pcall(require "edut".setup, {commands = {
  {"partial", execute = function() print("EXECUTED") end},
  {"broken", flags = {["--bad"] = -1}, execute = function() end},
}})
assert(not ok)
''')
        self.run_cli("partial", success=False)


if __name__ == "__main__":
    unittest.main()
