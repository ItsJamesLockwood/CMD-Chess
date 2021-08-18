# CMD-Chess
An object-oriented command line chess program with several advanced features.

This project was an attempt to create a chess game that can be interfaced with using the command line and was designed to ensure cross-platform compatibility (i.e. works on Windows and Linux-based systems).

To compile the project, the ```main.cpp``` file must be built and compiled to include the cpp modules and the header files. To compile, simply run the following in the CMD:

```g++ main.cpp -I <path-to-folder containing .cpp and .h files> -o main```

e.g. if files are contained in ```D:\Chess```, then run:

```g++ main.cpp -I D:\Chess -o main```

Once the application is built, simple run the file or executable. The interface is shown below.

![image](https://user-images.githubusercontent.com/33159939/129890358-f22bc28f-120b-474a-bdc3-10370e9ebd90.png)

To make a move, simply provide the [algebraic notation](https://en.wikipedia.org/wiki/Algebraic_notation_(chess)) for the move and hit Enter. Example: ```e4 + Enter```.


Provided the move was correct, i.e. it followed the conditions listed below, the program will automatically infer which piece needs moving, execute the move, and switch to the opposite player.

Conditions for a move to be valid:
- It follows standard algebraic notation (note that for castling, both ```O-O``` and ```OO``` are accepted for a castling move).
- There is no ambiguity in the move (i.e. there are no two piece that could feasibly execute the desired move).
- The move does not leave the player's king in check.

After the example move of pawn to e4, the board is updated (specifically, the screen is cleared and an updated version is printed at the top of the screen).

![image](https://user-images.githubusercontent.com/33159939/129891166-9b1a18e6-6d1c-4f4b-9b70-a05a84e3a864.png)

Note, that a number of features have been planned out, and their options are currently shown in the program despite their not being implemented in the current version. These features are planned for future updates.
