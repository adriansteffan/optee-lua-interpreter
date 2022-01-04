# Optee Lua Runtime Environment

This is a proof of concept Lua interpreter that can run on both the trusted and non trusted side of OPTEE OS (or any other secure computing environment that implements the Global Platform API with minor changes). It allows for the invokation of scripts between the two sides and allows for pure Lua applications to utilize the OPTEE trusted execution environment.

As this is just a poc to evaluate the capabilities of dynamic code interpreatation in a TEE, it is not recommended to be used in any security critical context as of now.

(This app has only been tested on a Raspberry Pi 3 Model B so far.)

This source code is part of my [thesis](https://adriansteffan.com/pdf/bthesis.pdf) done at the [TUM Chair of Connected Mobility](https://www.in.tum.de/cm/home/).



## Getting Started


### Prerequisites

To get an OPTEE environment running please refer to the [official documentation](https://optee.readthedocs.io/en/latest/).  


To encrypt and sign your own Lua applications, you will need to have both python3 and python3-pip installed. 
To get the necessary python packages, run 
```
pip3 install -r requirements.txt
```
in the root directory.

### Building

As this builds on the hello_world example of optee, you can refer to the [example documentation](https://optee.readthedocs.io/en/latest/building/gits/optee_examples/optee_examples.html) for building instructions.
See ``build.sh`` for example build commands.

Move the generated .ta file to the folder in your installation where trusted applications are located. The generated host binary can reside anywhere on the system running OPTEE.

## Usage

A Lua application built for use with this interporeter could look like this:

```
example_lua_app/
├── host
│   └── main.lua                Entrypoint for rich os lua script
│
└── ta     
    ├── ta_function1.lua        Lua function to be run in the TA
    └── ta_function2.lua
```

To encrypt the ta parts of the application, put the ```example_lua_app``` folder in the same directory as the ```encrypt_lua.py``` and run
```
python3 encrypt_lua.py example_lua_app
```
This will generate .luata files in the /ta/ folder of your application.

At the moment, both encrypted and non encrypted lua files can be run in the TA for testing purposes. In a real world scenario, the plaintext variant should be disabled, as to prevent the execution of unchecked code in the TA.

To run the application, put the ``` example_lua_app``` folder in the same directory as the ```invoke_lua_interpeter``` binary on your target system.
Then run 
```
./invoke_lua_interpreter [-su] example_lua_app
```

```
optional arguments:
  -s                Always invoke lua scripts saved in the secure TA 
                    storage instead of resending them when doing a TA call
                    (default: send in the lua script every time)

  -u                use the plaintext .lua files for execution in the 
                    trusted environment (default: use the encrypted .luata files)
 
```
to execute your application.



See the example application in the repo for some example code. A more in depth explanation of the API will follow later.

## Some things to note

As this is heavily wip, there are still some caveats to using the interpreter:

* At the moment, passing of arbitrary datatypes to the lua calls only works in the direction Rich OS -> TA and is limited to integers and strings for TA -> Rich OS
* The system for passing arbitrary arguments is very hacky at the moment and is in need of a rewrite later on.
* The code needs cleanup in some places and memory managemant is quite messy.
* There is no system in place for testing and benchmarking the system, which is the next point on the TODO list.
* This documentation is quite barebones and needs ro be extended in the future.


## Built With

* [OP-TEE](https://www.op-tee.org/)
* [Lua](https://www.lua.org/home.html)

## Authors

* **Adrian Steffan**

## Acknowledgments

Kudos to peter [Peter Feifan Chen](https://github.com/peterfeifanchen) for providing the modified version of the lua interpreter that is able to run in a trusted application. 
The code can be found in the ```lua``` directory (bar the "extensions" folder, which contains code written by me exclusively ). 

The project structure is based on the [hello_world_example](https://github.com/linaro-swg/optee_examples/tree/master/hello_world) by [Linaro](https://github.com/linaro-swg).
