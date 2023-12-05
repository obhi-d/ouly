# SCLI Specification

SCLI is acronym for "Structured Command Line Interaface".
The library provides tools to interpret command line that pertains to this specification.

## Document Grammar

### Regions

A region is a section of the document that is either parsed for commands, or added as a text section.
A new region begings with the `--` character at the start of a line.

```
region : -- (region_id|"cmds") ':' region_name --
```

A region consists of an `region_id` and a `region_name`. If the `region_id` is named `cmds`, it is parsed for commands, 
otherwise it is assumed to be a text region segment.

```example

-- my_text_reg : first text region --

This is just text

-- code : script_name --

this_is a command;

```

### Cmd Region

A command region contains one or more statements

```
statement : (command)*
```

### Command
A command is a named entitiy that optionally accepts parameters and a block of other internal commands.

```
command : command_name `:`? parameters? ( ';' | command_block? )
```



```example

command parameter1 [parameter2, parameter3, [named_1=something, named_2=something_else] ]
  param_name4="parameter_4 with spaces" param_name5=parameter_5
{
  other_command : with a colon to seperate parameters;
}

```

#### Command Name

Command name follows this identifier rules:

```
 command_name : [a-zA-Z0-9\._@*/\\]([a-zA-Z0-9\._@*/\\]|'-')*
```

Example name :

```example

my-new-command@007 : 

```

### Command Parameters

Parameters are either string, or non space seperated character blob, they can also be lists of parameter group.
Parameters can be named (with a keyword), with assignment operator
Positional arguments must be used before keyword arguments. 

```
parameters : (parameter)+
parameter : (parameter_name=)?parameter_value 
parameter_name : [a-zA-Z0-9\._@*/\\]([a-zA-Z0-9\._@*/\\]|'-')*
parameter_value : '"' .* '"' | ([a-zA-Z0-9\._@*/\\]|'-')* | '[' parameters ']'
```
 
 ## Interpreter

 Interperter works by utilizing a command registry. A command registry consists of command definition class metadata.
 A command definition is done using a custom class.

 ```c++

 
struct myCommandContext;
enum class myEnum
{
eA, eB
};

struct myParamCtx
{
  int other;

};

struct myCommand
{
  int  myStuff;
  myEnum myEnum;

  // called after filling up parameters
  bool execute(scli&) noexcept 
  {}
  // called before entering a block
  bool enter(scli&) noexcept {}
  //
  void exit(scli&) noexcept {}

  constexpr static auto reflect() noexcept
  {
    return acl::bind(acl::bind<"myStuff", &myCommand::myStuff>(), acl::bind<"myEnum", &myCommand::myEnum>());
  }
};

struct myRegionHandler
{
  enum region_id_t
  {
    code_reg
  };
  // called when entering a region
  bool enter(scli&, region_id_t t) noexcept {}
};

struct myTextHandler
{  
  // called when entering a region
  bool enter(scli&, std::string_view name, text_content&&) noexcept {}
};


 ```

 ### Building Registry

 ```c++

 scli::builder registry;
 registry
   [acl::reg<"root", myRegionHandler>]
     - acl::cmd<"my-command", myCommand>
     - acl::cmd<"my-command2", myCommand2>
     + acl::cmd<"my-command-block", myCommandBlock>
       - acl::cmd<"my-command", myCommand>
       - acl::endl
     - ...
 ```

