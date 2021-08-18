#include <string>
#include <vector>
#include <deque>

#include "pieces.h"
#include "pieces.cpp"

#include "board.h"
#include "utils.cpp"

#pragma once

// Define class with member functions to play a game of chess
class chess
{
    private:
        board chess_board{piece::get_locations()}; 
        chess_vars::game_option current_option {chess_vars::p_v_p}; // Play option: currently only pvp is supported
        chess_vars::player_color main_player {chess_vars::white}; // Which player to print on bottom of board
        chess_vars::player_color current_player {chess_vars::white}; // Which player is currently active
        chess_vars::setup setup_type { chess_vars::default_board }; // Allows for loading of previous games
        chess_vars::game_status current_status { chess_vars::game_on }; // Track if game is on or over
        chess_vars::game_outcome outcome { chess_vars::ongoing }; // Track the outcome of a game
        std::map<position, piece*> loaded_positions; // Load positions from previous game to the board
        std::map<position, piece*>* occupied {piece::get_locations()}; // Track locations of pieces on the board
        std::map<position, std::vector<piece*>>* accessible_squares {piece::get_destinations()}; // Track the pieces which can access any given square

        std::map<chess_vars::player_color, std::deque<std::string>> premoves; // Allow players to store moves
        std::map<chess_vars::player_color, king*>* the_kings; // Need to track kings' position to check for checks and checkmate
        std::map<chess_vars::player_color, bool> is_checked; // Track status of kings

        std::string save_location{"foobar.txt"}; // Default name for the savefile

        chess_vars::request current_request; // Current user request type for a move
        bool is_ready_status {false}; 
        bool initialisation_requested {true};
        bool want_to_resume {false};

    public:
        chess()
        {
            // While troubleshooting: option to load some basic premoves
            
            //premoves[chess_vars::white] = {"g4","f3","g5","g6","gxf","fxg"};//
            //premoves[chess_vars::black] = {"e6","b5","b4","b3","Ke7","bxc"};
            
            // premoves[chess_vars::white] = { "e4", "Nh3", "Bc4","oo","f4","f5","Qe2"};
            // premoves[chess_vars::black] = {"d5", "c6","a5","dxe","e3","e2"};//,"f1R"};
            // Anatoly Karpov vs Veselin Topalov : https://www.chessgames.com/perl/chessgame?gid=1069169
            // premoves[chess_vars::white] = {"d4","c4","Nf3","Nxd4","g3","Bg2","Nb3","Nc3","O-O","Bf4","e3","exf4","Qd2","Rfe1","h4","h5","hxg6","Nc5","Qxd7","Rxe6","Rxg6","Qe6","Bxc6","cxb5","Ne4","bxa6","Rd1","Rxd4","Qf6","Qxg6","Qe8","Qe5","Nf6","Be8","Qxc5","Qxa7","Bh5","b3","Kg2"};
            // premoves[chess_vars::black] = {"Nf6","c5","cxd4","e6","Nc6","Bc5","Be7","OO", "d6","Nh5","Nxf4","Bd7","Qb8","g6","a6","b5","hxg6","dxc5","Rc8","Ra7","fxg6", "Kg7","Rd8","Bf6","Bd4","Qb6","Qxa6","Rxd4","Kg8","Kf8","Kg7","Kg8","Kf7","Kf8","Qd6","Qxf6","Rd2","Rb2"};
        };
        ~chess(){};
        void load_menu();
        void ask_game_option();
        void ask_path();
        void ask_for_move();
        chess_vars::game_option get_option(); // might have to make these public
        void load_game();
        void save_game();
        void initialise_game();
        chess_vars::request get_request();
        chess_vars::setup get_setup();
        void undo_last_move();
        void generate_moves();
        void generate_threats();
        void update_game_status();
        bool over();
        bool is_ready();
        bool keep_going(){return true;}
        bool resume_game();
        bool want_initialisation();
        bool make_move(move_request, piece*, piece*);
        //TS:
        void print_accessible_squares();
        void print_board();
};

// For troubleshooting purposes only
void chess::print_accessible_squares()
{
    std::cout<< "Printing accessible squares:"<<std::endl;
    for (std::map<position, std::vector<piece*>>::iterator iter = (*accessible_squares).begin(); iter != (*accessible_squares).end(); ++iter){
        for (std::vector<piece*>::iterator it2 = (iter->second).begin(); it2 < (iter->second).end(); ++it2){
            if ( (*it2)->get_owner()!=current_player) continue; 
            std::cout<<"Destination: "<<iter->first;
            std::cout<< "\tPiece: "<<piece_to_char( (*it2)->get_abbrev());
            std::cout<< "\tOwner: "<< (*it2)->get_owner()<<"\tCurrent location: "<< (*it2)->location()<<std::endl;

        }
    }
}