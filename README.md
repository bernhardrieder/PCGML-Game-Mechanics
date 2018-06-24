# Using Procedural Content Generation via Machine Learning as a Game Mechanic
This repository is all about my master thesis addressing the open problem of "Using Procedural Content Generation via Machine Learning as a Game Mechanic," and is written for the study program "Game Engineering and Simulation Technologies" at the University of Applied Sciences Technikum Wien, Vienna, Austria.
Summerville et al. initially introduced the concept of using PCGML for, e.g., level generation and also other application. One leading open issue stated in the paper about PCGML is the use of PCGMl as a game mechanic which is addressed by this thesis.

## Repository Breakdown
There are two main projects in this repository:
- The master thesis, written in LaTeX.
- A prototype game scenario which uses Procedural Content Generation via Machine Learning (PCGML) as a game mechanic. The prototype folder contains the actual Unreal Engine 4 project and a test environment in which I tested the Python scripts before using them in the engine.

## Playable Prototype
Unfortunately, it is not possible to package the current version of the prototype due to a packaging error in the Unreal Engine Python plugin fork. Therefore, to test the prototype you need to follow the next chapter.

# Getting Started with the Project
## Requirements
- Unreal Engine 4.19.2

## How to Start and Play?
- Clone the repository
- Download "tensorflow-ue4.19-v0.8.0-cpu.7z" from https://github.com/getnamo/tensorflow-ue4/releases and copy/replace the binaries and third party folders of the plugins into/with their respective folders. For example, the binaries folder of the UnrealEnginePython plugin in the .7z folder goes into "..\Game Mechanic Prototype\UE4\Plugins\UnrealEnginePython". This embeds python into your project and you don't need to install it separately.
- Start the project and make sure you can activate the Python Console in "Window/Developer Tools/Python Console". This indicates that the Python plugin is enabled and works.
- Check the Python console and wait until all TensorFlow dependencies are installed. For more information, check out https://github.com/getnamo/tensorflow-ue4#installation--setup. Basically, it takes a maximum of 5 minutes and then you should see the following message in your logs: ```Successfully installed absl-py-0.2.2 astor-0.6.2 bleach-1.5.0 gast-0.2.0 grpcio-1.12.1 html5lib-0.9999999 markdown-2.6.11 numpy-1.14.5 protobuf-3.6.0 six-1.11.0 tensorboard-1.8.0 tensorflow-1.8.0 termcolor-1.1.0 werkzeug-0.14.1```
- You should be able to run the game now. To make sure everything works, open the weapon generator blueprint located in "Content/Blueprints/BP_TensorFlowWeaponGenerator". It should take a while to open, and no error message should appear in the logs. That means you are ready to go! 

## Known Issues
- TensorFlowComponent not found: This can happen if your path of the project is too long. Quick fix: move your project to another directory.
