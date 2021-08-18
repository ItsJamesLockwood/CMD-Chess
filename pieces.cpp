#include <map>

#include "pieces.h"
#include "position.h"

#pragma once


// Initialise static variables and functions
std::map<position, piece*> piece::occupied_squares{};
std::map<position, std::vector<piece*>> piece::destinations{};
std::map<chess_vars::player_color, int> piece::piece_count{}; // will probs need to set ints to 0.
std::map<chess_vars::player_color,std::vector<position>> piece::threats{}, piece::pinners{}, piece::defences{};


// Pawn variables
bool piece::legal_en_passant { false };
bool piece::move_is_en_passant{ false };
position piece::capture {position(0,0)};
void piece::set_legal_en_passant(bool e_p_status, position pawn_position)
{
    legal_en_passant = e_p_status;
    move_is_en_passant = false;
    if (e_p_status){
        capture = pawn_position;
    }
}



std::map<position, piece*>* piece::get_locations()
{
    return &occupied_squares;
};

std::map<position, std::vector<piece*>>* piece::get_destinations()
{
    return &destinations;
};
void piece::reset_occupied_spaces()
{
    std::cout<<"--> Resetting occupied_squares..."<<std::endl;
    for (int x{1}; x<=8; x++){
        for (int y{1}; y<=8; y++){
            if ( occupied_squares.count(position(x,y))){
                piece *temp {occupied_squares.at(position(x,y))};
                chess_vars::player_color col {(*temp).get_owner()};
                chess_vars::piece_type type {(*temp).get_abbrev()};
                position pos_ {(*temp).location()};
                delete temp;
                occupied_squares.erase(position(x,y));
            }
        }
    }

    occupied_squares.clear();
    std::cout<<"\tDone resetting."<<std::endl;
}

void piece::reset_threats(chess_vars::player_color current_player)
{
    threats[current_player].clear();
    defences[current_player].clear();
    pinners[current_player].clear();

    // Or threats.erase(owner) if we want to remove that key (bad idea?)
}

bool piece::is_threatened(piece* piece_ptr)
{
    chess_vars::player_color enemy_color {switch_player(piece_ptr->get_owner())};
    position pos {piece_ptr->location()};
    return is_in(threats[enemy_color], pos);
}

piece* piece_initialiser(chess_vars::player_color color, chess_vars::piece_type type, position pos)
{
    piece* piece_ptr;
    switch (type)
    {
    case chess_vars::pawn:
        piece_ptr = new pawn(color, type, pos);
        break;
    case chess_vars::rook:
        piece_ptr = new rook(color, type, pos);
        break;
    case chess_vars::knight:
        piece_ptr = new knight(color, type, pos);
        break;
    case chess_vars::bishop:
        piece_ptr = new bishop(color, type, pos);
        break;
    case chess_vars::king:
        piece_ptr = new king(color, type, pos);
        break;
    case chess_vars::queen:
        piece_ptr = new queen(color, type, pos);
        break;
    default:
        throw InvalidPiece();
    }
    return piece_ptr;
}
