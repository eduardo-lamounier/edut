local function load_commands(modules)
  local commands = {}

  for _, module_name in ipairs(modules) do
    local definitions = require(module_name)

    assert(
      type(definitions) == "table",
      module_name .. " must return a table of commands"
    )

    for _, command in ipairs(definitions) do
      table.insert(commands, command)
    end
  end

  return commands
end

require("edut").setup {
  commands = load_commands { "scripting" },
}

-- if you wish to customize something, do it here or in a
-- new .lua file in lua/ and require it
