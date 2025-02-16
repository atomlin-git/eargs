###### simple argument parser, supports both a string with arguments and an array from main

###### example of use:
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

###### todo:
> refactory <br>
> write more tests
