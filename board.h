// Interface for the board class, part of the C++ Chess Project.

#include <string>
#include <vector>
#include <map>

#include "position.h"
#include "pieces.h"
#include "utils.cpp"

#pragma once


// Class for printing the chess board and containing the pieces.
class board
{
    private:
        std::map<chess_vars::player_color, std::vector<piece*>> all_pieces;
        std::map<position, piece*> *occupied_spaces;

        
        std::string main_player{"white"}; // Player at the bottom of the board
        std::map<chess_vars::player_color, king*> kings;

    public:
        // Only parametrised constructor is provided
        //board() = default;
        board(std::map<position, piece*> *piece_ptr): occupied_spaces{piece_ptr} {}
        
        // Default layout of chess board
        void initialise_board();
        void change_main_player(std::string);
        std::map<position, piece*>* get_locations() const
        {
            // Only returns pointer!
            return occupied_spaces;
        }
        std::map<chess_vars::player_color, king*> &get_the_kings() 
        {
            return kings;
        }
        
        // Custom board setup
        void load_board(std::map<position, piece*> game_positions, std::map<chess_vars::player_color, king*> kings);
        void print_board();
};

void board::initialise_board()
{
    // Initialise top and bottom pawns
    for (int i{1}; i<=8; i++){
        pawn *w_pawn = new pawn {chess_vars::white,chess_vars::pawn, position(i,2)};
        pawn *b_pawn = new pawn{chess_vars::black,chess_vars::pawn,position(i,7)};
        all_pieces[chess_vars::white].push_back(w_pawn);    
        all_pieces[chess_vars::black].push_back(b_pawn);    
        
    }
    // Initialise other pieces
    chess_vars::player_color current_color{chess_vars::white};
    std::vector<piece*> back_rank_pieces;
    int rank{1};
    for (int c{}; c<2; c++){
        rook *rook_l = new rook (current_color,chess_vars::rook,position(1,rank));
        knight *knight_l = new knight {current_color,chess_vars::knight,position(2,rank)};
        bishop *bishop_l = new bishop {current_color,chess_vars::bishop,position(3,rank)};
        queen *the_queen = new queen {current_color,chess_vars::queen,position(4,rank)};
        king *the_king = new king {current_color,chess_vars::king,position(5,rank)};
        bishop *bishop_r = new bishop {current_color,chess_vars::bishop,position(6,rank)};
        knight *knight_r = new knight {current_color,chess_vars::knight,position(7,rank)};
        rook *rook_r = new rook {current_color,chess_vars::rook,position(8,rank)};
        back_rank_pieces = {rook_l, knight_l, bishop_l, the_queen, the_king, bishop_r, rook_r,knight_r};
        all_pieces[current_color].insert( all_pieces[current_color].end(), back_rank_pieces.begin(), back_rank_pieces.end());
        kings[current_color] = the_king;

        current_color = chess_vars::black;
        rank += 7;
    }
}

void board::load_board(std::map<position, piece*> game_positions, std::map<chess_vars::player_color, king*> kings_)
{
    (*occupied_spaces) = game_positions;
    kings = kings_;
}
void board::change_main_player(std::string new_player){
    // Requires translation over to enum system
    main_player = new_player;
}

void board::print_board()
{
    int depth{1}; // 0: white; 1: black
    std::string line(depth, ' ');
    std::string icon;
    chess_vars::piece_type abbrev;
    chess_vars::player_color piece_color;
    std::string extra_spaces {};
    int icon_offset{0};
#if USEICONS
    extra_spaces = " ";
    icon_offset = 1;
#endif
    // Define iterators here  
    std::vector<int> y_coords;
    for (int i{1}; i<=8; i++){
        y_coords.push_back(i);
    }
    
    if (main_player=="white"){
        reverse(y_coords.begin(), y_coords.end());
    }
    std::vector<int>::iterator y_begin {y_coords.begin()} , y_end {y_coords.end()}, y_it{};

    int j{};
    for (y_it=y_begin; y_it<y_end; ++y_it){ // print top of board first, and make our way down
        j = (*y_it);        
        for (int d{}; d<depth; d++){
            for (int i{1}; i<=8;i++){
                if (depth-(d+ (2-icon_offset) -(depth + icon_offset)%2)!=d){
                    icon = " " + extra_spaces;
                } else if (occupied_spaces->count(position(i,j))==1) {
                    piece_color = (*occupied_spaces)[position(i,j)]->get_owner();
                    abbrev = (*occupied_spaces)[position(i,j)]->get_abbrev();
                    icon = format_icon(abbrev,piece_color);
                } else {
                    icon = " " + extra_spaces;
                } //TS: could probably reduce to one if statement 

                if (i%2 ^ j%2 ==1){ // Case where square is white square
                    std::cout<< "\033[47m" << line << icon << line << "\033[0m";
                } else { // If not white, must be black square
                    std::cout<< "\033[41m" << line << icon << line << "\033[0m";
                }
                
            }
            std::cout<<std::endl;
        }
    }
}
