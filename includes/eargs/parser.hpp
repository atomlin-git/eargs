#pragma once

#include <list>
#include <string>
#include <vector>
#include <cassert>
#include <type_traits>

namespace eargs {
    enum types { string, integer, hex, boolean, empty };

    struct option {
        std::list<std::string> names = {};
        std::string description = "";
        std::string variable = "";
        eargs::types type = eargs::string;
        bool required = false;

        option(std::list<std::string> names_, std::string description_, eargs::types type_, bool required_) : names(names_), description(description_), type(type_), required(required_) {};
    };

    class parser {
        std::list<eargs::option> options;

        auto string_to_vec(std::string str) {
            std::vector<std::string> out = {};
            
            int start = 0;
            for(int i = 0; i < str.length(); i++) {
                if(str[i] == ' ') {
                    out.emplace_back(str.substr(start, i - start));
                    start = i + 1;
                };
            };

            out.emplace_back(str.substr(start, (str.length() - start) + 1));
            return out;
        };

        public:
            parser(std::list<eargs::option> options_) : options(options_) {};
            parser(eargs::option option) : options({option}) {};

            bool parse(std::string str) {
                char* args[sizeof(unsigned short)];
                auto parsed_str = this->string_to_vec(str);
                for(unsigned int i = 0; i < parsed_str.size(); i++) args[i + 1] = (char*)parsed_str[i].c_str();

                return this->parse(args, int(parsed_str.size() + 1));
            };

            bool parse(char* args[], int count) {
                if(count <= 1) return false;

                for(int i = 1; i < count; i++) {
                    for(auto& opt : options) {
                        if((args[i][0] == '-' && (i + 1) >= count) || (args[i][0] == '-' && args[i + 1][0] == '-')) {
                            for(auto& name : opt.names) {
                                if(args[i] == ("-" + name)) {
                                    opt.variable = "empty";
                                    break;
                                };
                            };

                            continue;
                        };

                        for(auto& name : opt.names) {
                            if(args[i] == ("-" + name)) {
                                opt.variable = args[i + 1];
                                i++;
                                break;
                            };
                        };
                    };
                };

                for(const auto& opt : this->options) {
                    if(opt.required && opt.variable == "") {
                        return false;
                    };
                };

                return true;
            }; 
            
            bool contains(std::string name) {
                for(const auto& opt : this->options) {
                    for(const auto& opt_name : opt.names) {
                        if(name == opt_name) {
                            return opt.variable != "";
                        };
                    };
                };

                return false;
            };

            bool print_help() {
                for(const auto& opt : this->options) {
                    std::string name = "";
                    for(const auto& opt_name : opt.names) name += opt_name + " ";

                    printf("%s\t: %s\t (required: %s)\n", name.c_str(), opt.description.c_str(), opt.required ? "true" : "false");
                };

                return true;
            };

            template<typename T>
                typename std::enable_if<std::is_same<T, bool>::value || std::is_same<T, int>::value || std::is_same<T, unsigned int>::value || std::is_same<T, long long>::value || std::is_same<T, unsigned long long>::value, T>::type get(std::string name) {
                    for(const auto& opt : this->options) {
                        for(const auto& opt_name : opt.names) {
                            if(name == opt_name) {
                                #ifdef _DEBUG
                                    assert((opt.type == eargs::types::boolean) || (opt.type == eargs::types::integer) || (opt.type == eargs::types::hex));
                                #endif

                                switch(opt.type) {
                                    case eargs::hex: return std::strtoul(opt.variable.c_str(), 0, 16);
                                    case eargs::integer: case eargs::boolean: return std::atoi(opt.variable.c_str());
                                };
                            };
                        };
                    };
                    
                    return 0;
                };

            template<typename T>
                typename std::enable_if<std::is_same<T, std::string>::value, T>::type get(std::string name) {
                    for(const auto& opt : this->options) {
                        for(const auto& opt_name : opt.names) {
                            if(name == opt_name) {
                                #ifdef _DEBUG
                                    assert(opt.type == eargs::types::string);
                                #endif
                                
                                return opt.variable;
                            };
                        };
                    };
                    
                    return "";
                };
    };
};