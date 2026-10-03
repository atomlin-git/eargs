Option names must be unique across definitions and within each alias list.
The parser constructor throws `std::logic_error` naming any duplicate.

## Enum choices

Declare a table of accepted spellings and their enum values. Different spellings
may map to the same enum value. The table owns its strings; it does not retain
references to the caller's data.

```cpp
enum class Color { unknown, red, blue };

eargs::parser parser = {{
    {{"c", "color"}, "color",
     eargs::choices<Color>{{
         {"red", Color::red}, {"r", Color::red}, {"blue", Color::blue}
     }, Color::unknown}}
}};

if (!parser.parse("--color r", eargs::parse_mode::strict)) {
    // parser.error() names the option and lists its accepted spellings.
}
const auto color = parser.get<Color>("color"); // Color::red
const auto suggestions = parser.complete("c", "r"); // {"red", "r"}
```

Invalid values fail parsing in both strict and permissive modes without
publishing partial values. `get<Enum>` requires the exact enum type declared by
the option; a type mismatch throws `std::logic_error`. String retrieval remains
available and returns the spelling that was supplied.

The second `choices` argument is an optional default used only when the option
was not supplied; it may be a sentinel such as `Color::unknown` outside the
accepted value table. Without an explicit default, retrieving an absent enum
option throws `std::logic_error`. A default does not satisfy a required option.
Unknown option names passed to `get<Enum>` throw `std::out_of_range`.

Empty tables and duplicate spellings throw `std::invalid_argument` during
definition. An explicitly declared empty spelling accepts `--option=` and
remains distinguishable from absence through `contains`.

`print_help()` lists accepted spellings, including value aliases. `complete`
returns spellings whose prefix matches exactly, in declaration order, and works
before parsing. Unknown options and options without enum choices produce no
suggestions. Shell integrations can use this API to obtain value completions.

## String fallbacks

`parser.get<std::string>("output", "default.bin")` returns the fallback only
when the option was not supplied (or is unknown). An explicit empty value such
as `--output=` remains empty. The returned string owns its data; the parser does
not retain the fallback's `std::string_view`.

###### example of use (learn more in the tests):
```c++
eargs::parser parser = {{
        {{"h", "help"}, "it print help", eargs::empty, false},
        {{"addr"}, "it get addr at hex", eargs::hex, false},
        {{"name"}, "it get person name", eargs::string, true},
        {{"phone"}, "it get phone number at integer", eargs::integer, true}
    }
};

if(!parser.parse(args, count)) {
    return printf("one of required arguments not found!\n");
};

if(parser.contains("help")) {
    return parser.print_help();
};

if(parser.contains("addr")) {
  printf("addr: %X\n", parser.get<int>("addr"));
};

printf("name: %s\n", parser.get<std::string>("name").c_str());
printf("phone: %d\n", parser.get<int>("phone"));
```
