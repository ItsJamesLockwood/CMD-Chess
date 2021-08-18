// C++ Final Project:
// Fully-working chess game with load and save functionalities.
// Author: James Lockwood
// Last udated: 24/05/2021


#include "game.cpp"


int main(){
    chess game;
    print_welcome(); 
    // While player wants to keep playing: loop
    while (game.keep_going()){
        // Menu: choose an option to proceed
        game.ask_game_option();   

        // If menu option requires a board initialisation: proceed 
        if (game.want_initialisation()){
            game.initialise_game(); // print current status of board: either new board or loaded game
            if (game.get_setup()==chess_vars::loaded_board){
                game.update_game_status(); // Required to initialise opponent's threats and check for checkmate/stalemate
            }
        } else if (game.resume_game()){ // If asking to resume a game in progress
            game.print_board();
        }

        // Main loop of the game: keeps looping until exit requested or game is over
        while (!game.over() && game.is_ready()){
            // Request algebraic move from player
            try {
                game.ask_for_move(); // inside: repeat until valid move or command
            } catch (EmptyQueueException& e){
                std::cerr << e.what() << "\n";
            } 
            // has to handle:
            // - if move: is this move possible
            // - if move: can only one piece make this move: if not return options
            // - if multiple moves: only check first one and store the next
            // - if request to save/exit/undo: check if valid command
            
            // If requesting to go back to menu: exit
            if (game.get_request()==chess_vars::menu){
                //TS
                std::cout<<"EXITING TO MENU!!"<<std::endl;
                break;
            }

            // Check if move lead to a winning scenario. Print the board and recaculate allowed moves and threats
            game.update_game_status();
        }
    }
    exit_message();
    return 0;
}