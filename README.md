# Readme and some notes here

## Build the project

Use 
```bash
xmake build
```
to build the project, and
```bash
xmake run
```
to run the project.

You may need
```bash
xmake project -k compile_commands
```
To generate compile_commands.

Use
```bash
xmake f -m debug
```
To change to debug mode.

## Project Architecture

### DisplayText

We are using `ftxui` as third party tui library.

## Now investigating into the original shard.

The singleton are 
```text
Debug, Base functionality
```

Abstract base class [interface] are:
```text
Game, Display, TextDisplay:Display, Sould, SouldBeep: Sound
```

We may want the following singleton:
```text
DisplayEngine, SoundEngine, Logger.
```
For the inheritance part, we need composition instead of inheritance.

- DisplayEngine will include DisplayBase. We can inheriting that DisplayText: DisplayBase.
  - Use example: DisplayBase* displayer = DisplayEngine::GetInstance().displayEngine()...
  - Also for SouldEngine....
  - Never inheritance a Singleton Child,,, I will find a way to solve that problem maybe? I think there need to be more meta programming and I'm not really familiar with that......