#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <map>

#include "position.h"
#include "utils.cpp"

#pragma once

template <typename T>
using player_map = typename std::map<std::string, std::vector<T>>;
typedef player_map<position> position_map;
//typedef std::map<std::string, std::vector<T>> player_map;
//typedef player_map<position> position_map;


class piece
{
    // May not be necessary in the end, but convenient nonetheless
    typedef player_map<piece*> piece_map;
    typedef std::map< std::string, std::map<position, piece*>> piece_lookup;
   
    protected:
        chess_vars::player_color owner;
        chess_vars::piece_type abbreviation;
        position current_position;
        int id; // distinguish between the pieces, may not be necessary in the end
        // List of all allowed moves: i.e. no jumping pieces, no capturing own pieces
        bool can_jump{false};
        int has_moved{0};

        // New approach for moves: must be made static TS!!
        std::vector<position> allowed_moves; // Piece by piece basis
        static std::map<chess_vars::player_color,std::vector<position>> threats, pinners, defences; // Concerns the opposition, i.e. no requirement to treat on piece-by-piece basis
        //piece_lookup threats,pinners,defences;
        //position_map allowed_moves;
        std::vector<position> increments;

        // variables for the pawn 
        static bool legal_en_passant;
        static bool move_is_en_passant;
        static position capture; // position of pawn to capture
        
        static std::map<position, piece*> occupied_squares; // Track all squares occupied by pieces
        static std::map<position, std::vector<piece*>> destinations; // Track all pieces that can access a given square
        static std::map<chess_vars::player_color, int> piece_count; // Track the number of pieces a player has (not relevant to functioning of code)
    public: 
        piece()=default;
        piece(chess_vars::player_color color, chess_vars::piece_type abbrev, position start_location) :
            owner{color}, abbreviation{abbrev}, current_position{start_location} 
        {
            if (current_position.is_valid()){
                occupied_squares[current_position] = this;
                piece_count[owner] ++;
            } else{
                // Provide some error catching here
                std::cerr<<"You gave an incorrect position when creating a new piece."<<std::endl;
            }
            if (abbreviation==chess_vars::nancy_rothwell){
                std::cerr<<"Something went wrong: why Nancy??"<<std::endl;
            }
        };
        
        virtual ~piece(){};
        virtual void generate_allowed_moves();
        virtual void generate_threats();
        piece &operator=(piece&);

        static std::map<position, piece*>* get_locations();
        static std::map<position, std::vector<piece*>>* get_destinations();
        static void reset_threats(chess_vars::player_color current_player);
        static void reset_occupied_spaces();
        static bool is_threatened(piece*);
        static void set_legal_en_passant(bool new_status, position weak_pawn);
        position location() const
        {
            return current_position;
        }
        chess_vars::player_color get_owner() const
        {
            return owner;
        }
        chess_vars::piece_type get_abbrev() const
        {
            return abbreviation;
        }
        int check_if_moved() const
        {
            return has_moved;
        }
        void set_moved(int moved_status)
        {
            has_moved = moved_status;
        }
        // Return (true, ptr to captured piece) if a piece was captured, else return (false, random pointer). //TS: probs not best practise to leave uninitialised
        std::pair<bool,piece*> move(position new_pos)
        {
            std::pair<bool, piece*> captured_piece; 
            position temp(6,5); //TS
            captured_piece.first = false;
            if (new_pos == current_position) { // Sanity check: can't move to same position
                std::cerr<<"CRITICAL: you moved to your current location for :"<<(*this)<<std::endl;
                exit(EXIT_FAILURE);
            }
            //TS segment
#ifdef DEBUGMODE
            if (abbreviation==chess_vars::pawn){
                std::cout<<"WARNING: Request to move a pawn. First printing allowed moves:"<<std::endl;
                this->print_allowed_moves();
                std::cout<<"PAWN MOVE: requested ->"<<new_pos<<std::endl;
            }
#endif
            // Is the move allowed?
            if (is_in(allowed_moves, new_pos)){
                // Is the move legal?
                occupied_squares.erase(current_position);
                
                // Before updating new_pos: check if capture occurs? Will need to delete cpatured piece at some point
                if (occupied_squares.count(new_pos)){
                    // Delete dynamic pointer to captured piece
                    if (occupied_squares[new_pos]->get_owner()==owner){
                        //throw error
                        // Sanity check: should not happen as should already have been checked
                        std::cerr<<"ERROR: asking to delete my own piece during a capture! Exiting...";
                        exit(EXIT_FAILURE);                        
                    }
                    captured_piece.first = true;
                    captured_piece.second = occupied_squares[new_pos];
                    //delete occupied_squares[new_pos]; // What if need to rollback??
                } else if (legal_en_passant && abbreviation==chess_vars::pawn){
                    int direction{1};
                    if (owner==chess_vars::black){
                        direction = -1;
                    }
                    position ep_pawn_pos{ new_pos.x(), new_pos.y()-direction};
                    if (1==occupied_squares.count(ep_pawn_pos)){
                        bool test_ep_type, test_ep_owner;
                        test_ep_owner = occupied_squares[ep_pawn_pos]->get_owner()!=owner;
                        test_ep_type = occupied_squares[ep_pawn_pos]->get_abbrev()==chess_vars::pawn;
                        if (test_ep_type && test_ep_owner){
                            captured_piece.first = true;
                            captured_piece.second = occupied_squares[ep_pawn_pos];
                            occupied_squares.erase(ep_pawn_pos);
                            std::cout<<"Confirmed En-passant -> returning piece to capture!"<<std::endl;
                            move_is_en_passant = true;
                        }
                    }
                }
                
                occupied_squares[new_pos] = this; 
                
                current_position = new_pos;
                has_moved ++;
                
            // Move is not allowed
            } else {
                std::cerr << "That move is not allowed!" <<std::endl;
                exit(EXIT_FAILURE);
            }
            
            return captured_piece;
        }

        // Undo a move if it was illegal
        void unmove(position old_pos, std::pair<bool,piece*> captured_piece)
        {
            // Note: alternative is to use a stored history of position to revert back, but is it useful outside of this application?
#ifdef VERBOSE
            std::cout<<"UNMOVE CALLED!"<<std::endl;
#endif
            // Is the previous location valid?
            if (old_pos.is_valid() && occupied_squares.count(old_pos)==0){ // Verify that the previous position is indeed empty and valid
                occupied_squares.erase(current_position); // Current position should no longer be occupied
                if (captured_piece.first){
                    if (move_is_en_passant){
                        int direction {1};
                        if (owner==chess_vars::black){
                            direction = -1;
                        }
                        occupied_squares[current_position + position(0,-direction)] = captured_piece.second;
                    } else {
                        occupied_squares[current_position] = captured_piece.second; // If piece was captured: replace it here
                    }
                }
                occupied_squares[old_pos] = this;
                current_position = old_pos;
                has_moved --;
            // Castling: rook is in king's previous spot
            } else if (old_pos.is_valid() && occupied_squares.count(old_pos)){
                
                bool test_1, test_2, test_3, test_4, test_5;
                test_1 = occupied_squares[old_pos]->get_owner()==owner; 
                test_2 = occupied_squares[old_pos]->get_abbrev()==chess_vars::rook;
                test_3 = abs(old_pos.x()-current_position.x())==1;
                test_4 = old_pos.y() == current_position.y();
                test_5 = abbreviation == chess_vars::king;
                
                if ( test_1 && test_2 && test_3 && test_4 && test_5){
                    // Check that neighbouring piece in previous spot is actually a rook and that we are a king
                    occupied_squares.erase(current_position); // Current position should no longer be occupied
                    if (captured_piece.first){
                        std::cerr<<"Move to undo involved a capture, yet it appears the last move was castling!"<<std::endl;
                        exit(EXIT_FAILURE);
                    }
                    occupied_squares[old_pos] = this;
                    current_position = old_pos;
                    has_moved --;
                } else{
                    // A test failed: no reason for previous spot to be occupied if not due to a castle
                    std::cerr << "Unable to revert to previous state (occupied, but not due to castling)." <<std::endl;
                    exit(EXIT_FAILURE);
                }
            // Unmove is not allowed: coding or user error
            } else {
                std::cerr << "Unable to revert to previous state (either invalid position, or that space isn't empty)." <<std::endl;
                exit(EXIT_FAILURE);
            }
            move_is_en_passant = false;
        }

        void print_allowed_moves() const
        {
#ifdef VERBOSE
            std::cout<<"Piece: "<< abbreviation <<"; Owner: "<<owner<<std::endl;
            std::cout<<"Count: "<<allowed_moves.size()<<std::endl;
            std::cout<<"Piece: "<< abbreviation << "; Number of moves: "<<allowed_moves.size()<<std::endl;
#endif
            for (auto pos_iterator{allowed_moves.begin()}; pos_iterator<allowed_moves.end();++pos_iterator){
                std::cout<< *pos_iterator <<std::endl;
            }
        }
        void print_threats() const
        {
            std::cout<<"Piece: "<< abbreviation << "; Number of moves: "<<threats.at(owner).size()<<std::endl;
            for (auto pos_iterator{threats.at(owner).begin()}; pos_iterator<threats.at(owner).end();++pos_iterator){
                std::cout<< *pos_iterator <<std::endl;
            }
        }      

        //Note: could return size_t instead
        int get_number_of_moves() const
        {
            return allowed_moves.size();
        }

        std::pair<std::vector<position>::iterator, std::vector<position>::iterator> get_allowed_iterators() 
        {
            std::vector<position>::iterator allowed_begin { allowed_moves.begin() };
            std::vector<position>::iterator allowed_end { allowed_moves.end() };
            return std::make_pair(allowed_begin , allowed_end);
        }

        //Friend functions
        friend std::ostream & operator<<(std::ostream &os, piece p);
};
piece &piece::operator=(piece &old_piece)
{
    if (&old_piece == this){
        return *this;
    }
    has_moved = old_piece.has_moved;
    current_position = old_piece.current_position;
    owner = old_piece.owner;
    abbreviation = old_piece.abbreviation;
    allowed_moves = old_piece.allowed_moves;
    return *this;
}

void piece::generate_allowed_moves()
{
    // First reset the allowed moves 
    allowed_moves.clear();
    //TS: std::cout<<"Generating allowed moves for: "<<color_to_char(owner)<<" "<<piece_to_char(abbreviation)<<std::endl;
    std::vector<position>::iterator inc_begin {increments.begin()};
    std::vector<position>::iterator inc_end {increments.end()};
    std::vector<position>::iterator inc_it {};
    for (inc_it=inc_begin; inc_it<inc_end; ++inc_it){
        position move{*inc_it};
        position increment{*inc_it};
        while ((current_position+move).is_valid() && occupied_squares.count(current_position+move)==0){
            allowed_moves.push_back(current_position+move);
            destinations[current_position+move].push_back(this);
            move = move + increment;
        }
        if (occupied_squares.count(current_position+move)==1){
            if (occupied_squares[current_position+move]->get_owner()!=owner){
                allowed_moves.push_back(current_position+move);
                destinations[current_position+move].push_back(this);
            }
        }
    }
}

void piece::generate_threats() //Could these not be combined into one function??
{
    std::vector<position>::iterator inc_begin {increments.begin()};
    std::vector<position>::iterator inc_end {increments.end()};
    std::vector<position>::iterator inc_it {};
    //TS: std::cout<<"Generating threats for "<<piece_to_char(abbreviation)<<" for player "<<owner<<std::endl;
          
    for (inc_it=inc_begin; inc_it<inc_end; ++inc_it){
        position move{*inc_it};
        position increment{*inc_it};
        //TS: std::cout<<piece_to_char(abbreviation)<<": "<<increment<<std::endl;
        while ((current_position+move).is_valid() && occupied_squares.count(current_position+move)==0){
            threats[owner].push_back(current_position+move);
            move = move + increment;
        }
        if (occupied_squares.count(current_position+move)==1){
            if (occupied_squares[current_position+move]->get_owner()==owner){
                defences[owner].push_back(current_position+move);
            } else {
                threats[owner].push_back(current_position+move);
                if ( (current_position+move+increment).is_valid() && occupied_squares.count(current_position+move+increment)==1){
                    if (occupied_squares[current_position+move+increment]->get_owner()!=owner){
                        pinners[owner].push_back( current_position + move + increment);
                    }
                }
            }
        }
    }
}

// House the move request information
struct move_request
{
    chess_vars::piece_type piece_id;
    position start { position(0,0)};
    position end { position(0,0)};
    position castle_end { position(0,0)};
    bool capture{false};
    bool valid{false};
    bool k_castle{false};
    bool q_castle{false};
    bool draw_offer{false};
    bool promotion{false};
    chess_vars::piece_type id{ chess_vars::nancy_rothwell };
    bool pawn_attack{false};
    bool pawn_move{false};
    chess_vars::request type{chess_vars::move};
};
std::ostream & operator<<(std::ostream &os, move_request m){
    os <<"Move for piece '"<<m.id<<"':"<<std::endl;
    if (!m.valid){
        os <<"Invalid move;"<<std::endl;
        return os;
    }
    os <<"Start: "<<m.start <<std::endl
        << "End: "<<m.end <<std::endl
        << "Castle: "<< (m.k_castle || m.q_castle) <<std::endl
        << "Draw offer: "<<m.draw_offer <<std::endl
        << "Pawn move: "<<m.pawn_move <<std::endl;
    return os;
}

std::ostream & operator<<(std::ostream &os, piece p)
{
    os << "Piece: "<<piece_to_char(p.abbreviation)<<" owned by "<<p.owner<<std::endl;
    os << "\tAt location: "<< p.current_position << std::endl;
    return os;
}

class pawn: public piece
{
    private:
        bool double_jumped{false};
    public:
        pawn() : piece{} {}
        pawn(chess_vars::player_color color, chess_vars::piece_type abbrev, position start_location) : 
            piece{color, abbrev, start_location} 
        {
            //Debug option: if starting board has a pawn starting in rank>2: set has_moved =true;
            if (owner==chess_vars::white){
                if (start_location.y()>2 && start_location.y()<8){
                    has_moved = 1;
                } else if (start_location.y()==8){
                    // ask user for promotion piece
                } else { // Other options include: y=1 or y invalid, both of which should be rejected
                    // Warn that placement is incorrect for a pawn
                }
            }
        };
        
        void generate_allowed_moves()
        {
            // First reset the allowed moves 
            allowed_moves.clear();
            //TS: std::cout<<"Generating allowed moves for: "<<color_to_char(owner)<<" "<<piece_to_char(abbreviation)<<std::endl;
            position pos_1, pos_2;
            int direction;
            if (owner==chess_vars::white){
                direction = 1;    
            } else {
                direction = -1;
            }
            pos_1 = position(current_position.x(),current_position.y() + direction);
            pos_2 = position(current_position.x(),current_position.y() + 2*direction);
            
            // Check if pawn could move forward
            if (pos_1.is_valid() && occupied_squares.count(pos_1)==0) {
                allowed_moves.push_back(pos_1);
                destinations[pos_1].push_back(this);            
                if (has_moved==0 && occupied_squares.count(pos_2)==0){
                    allowed_moves.push_back(pos_2);
                    destinations[pos_2].push_back(this);
                }
            }
            // Now check if could capture neighbouring piece
            for (int d{}; d<2; d++){
                pos_1 = position(current_position.x()-1 + 2*d, current_position.y()+direction); // Only select diagonal squares next to pawn
                if (occupied_squares.count(pos_1) && pos_1.is_valid()){
                    if (occupied_squares.at(pos_1)->get_owner()!=owner){
                        allowed_moves.push_back(pos_1);
#ifdef DEBUGMODE
                        std::cout<<"Capture at "<<pos_1.x()<<":" << pos_1.y()<<" allowed -> added to legal moves for this pawn"<<std::endl;
#endif
                        destinations[pos_1].push_back(this);
                    }
                }        
            }

            // Now check if diagonal spaces are occupied
            if (legal_en_passant){
                // Check if capture position is valid (sanity) + if more than 1 file over + if more than one rank over
                if (!capture.is_valid() || abs(capture.x()-current_position.x())!=1 || (capture.y() != current_position.y())){
                    // Error handling for incorrectly provided en-passant
                } else {
                    int direction{1};
                    if (owner==chess_vars::black){
                        direction=-1;
                    }
                    allowed_moves.push_back(capture + position(0,direction) );
                    std::cout<<"En-passant legal -> added to legal moves for this pawn"<<std::endl;
                    destinations[capture + position(0,direction)].push_back(this);
                }
            }
            // Note: pawn promotion (i.e. converting P to another piece) will have to be handled in-game.
#ifdef DEBUGMODE
            std::cout<<"Pawn moves ("<<current_position.x()<< ":"<<current_position.y()<<"): "<<allowed_moves.size()<<" allowed moves"<<std::endl;
#endif
        }
        void generate_threats()
        {
            position pos;
            int direction;
            if (owner==chess_vars::white){
                direction = 1;    
            } else {
                direction = -1;
            }

            // Check if diagonal squares are in the board. If so, the pawn will threaten/defend them
            for (int d{}; d<2; d++){
                pos = position(current_position.x()-1 + 2*d, current_position.y()+direction); // Only select diagonal squares next to pawn
                if (occupied_squares.count(pos) && pos.is_valid()){
                    if (occupied_squares.at(pos)->get_owner()==owner){
                        defences[owner].push_back(pos);
                    } else {
                        threats[owner].push_back(pos);
                    }
                }        
            }
            // Check for en_passant threat. TS: does the threat exist without the previous pawn move? As in, should the threat still be noted? Probably... TBD
            if (legal_en_passant){
                if (!capture.is_valid() || abs(capture.x()-current_position.x())!=1 || (capture.y()- direction*current_position.y())!=1){
                    // Error handling for inccorect en-passant capture
                } else{
                    threats[owner].push_back(capture);
                }
            }
        }
};

class knight: public piece
{
    public:
        knight() : piece{} {}
        knight(chess_vars::player_color color, chess_vars::piece_type abbrev, position start_location) : 
            piece{color, abbrev, start_location} 
        {
            can_jump=true;
            increments = { position(1,2), position(1,-2), position(-1,2), position(-1,-2),
                position(2,1), position(2,-1), position(-2,1), position(-2,-1)};
        };
        void generate_allowed_moves()
        {
            std::vector<position>::iterator inc_begin {increments.begin()};
            std::vector<position>::iterator inc_end {increments.end()};
            std::vector<position>::iterator inc_it {};
            for (inc_it=inc_begin; inc_it<inc_end; ++inc_it){
                position increment{*inc_it};
                if ( (current_position+increment).is_valid()){
                    if ( occupied_squares.count(current_position+increment)){
                        if (occupied_squares[current_position + increment]->get_owner()==owner){
                            continue;
                        } else {
                            allowed_moves.push_back( current_position + increment);
                            destinations[current_position+increment].push_back(this);
                        }
                    } else{
                        allowed_moves.push_back( current_position + increment);
                        destinations[current_position+increment].push_back(this);
                    }
                }
            }
        }
        void generate_threats()
        {
            std::vector<position>::iterator inc_begin {increments.begin()};
            std::vector<position>::iterator inc_end {increments.end()};
            std::vector<position>::iterator inc_it {};
            for (inc_it=inc_begin; inc_it<inc_end; ++inc_it){
                position increment{*inc_it};
                if ( (current_position+increment).is_valid()){
                    if (occupied_squares.count(current_position+increment)==0){
                        threats[owner].push_back(current_position+increment);
                    } else if (occupied_squares.at(current_position+increment)->get_owner()==owner){
                        defences[owner].push_back(current_position+increment);
                    } else{
                        threats[owner].push_back(current_position+increment);
                    }//TS: could keep the second if statement and append all other cases to threats
                }
            } 
            
        }
};

class bishop: public piece
{
    public:
        bishop() : piece{} {}
        bishop(chess_vars::player_color color, chess_vars::piece_type abbrev, position start_location) : 
            piece{color, abbrev, start_location} 
        {
            increments = {position(1,1), position(1,-1), position(-1,-1), position(-1,1)};    
        }
        
};

class rook: public piece
{
    public:
        rook() : piece{} {}
        rook(chess_vars::player_color color, chess_vars::piece_type abbrev, position start_location) : 
            piece{color, abbrev, start_location} 
        {
            increments = {position(1,0), position(-1,0), position(0,1), position(0,-1)};
        }

};

class queen: public piece
{
    public:
        // TS: functionality should only be kept while debugging. Remove from final product
        queen() : piece{} {}
        queen(chess_vars::player_color color, chess_vars::piece_type abbrev, position start_location) : 
            piece{color, abbrev, start_location} 
        {
            increments = {position(1,1), position(1,-1), position(-1,-1), position(-1,1), //diagonal moves
                position(1,0), position(-1,0), position(0,1), position(0,-1)}; // horizontal + vertical moves
        }
        
};

class king: public piece
{
    private:
        bool is_in_check;
        bool has_castled;
        chess_vars::check_status status{ chess_vars::nominal};
        chess_vars::castle legal_castle; //use enum to get the right integer: 0: no, 1: queen side, 2: king side, 3: both sides are legal
    public:
        king() : piece{} {}
        king(chess_vars::player_color color, chess_vars::piece_type abbrev, position start_location) : 
            piece{color, abbrev, start_location}, is_in_check{false} 
        {
            increments = {position(1,1), position(1,-1), position(-1,-1), position(-1,1), //diagonal moves
                position(1,0), position(-1,0), position(0,1), position(0,-1)}; // horizontal + vertical moves
        }
        void generate_allowed_moves()
        {
            //First erase allowed moves
            allowed_moves.clear();
            //TS: std::cout<<"Generating allowed moves for: "<<color_to_char(owner)<<" "<<piece_to_char(abbreviation)<<std::endl;
            
            std::vector<position>::iterator inc_begin {increments.begin()};
            std::vector<position>::iterator inc_end {increments.end()};
            std::vector<position>::iterator inc_it {};
            for (inc_it=inc_begin; inc_it<inc_end; ++inc_it){
                position increment{*inc_it};
                if ( (current_position+increment).is_valid()){
                    if ( occupied_squares.count(current_position+increment)){
                        if (occupied_squares[current_position + increment]->get_owner()==owner){
                            continue;
                        } else {
                            allowed_moves.push_back( current_position + increment);
                            destinations[current_position + increment].push_back(this);
                        }
                    } else{
                        allowed_moves.push_back( current_position + increment);
                        destinations[current_position + increment].push_back(this);
                    }
                }
            }
            // Add castling
            //TS: std::cout<<"\tChecking castling for: "<<color_to_char(owner)<<" "<<piece_to_char(abbreviation)<<std::endl;

            if (this->can_castle()>chess_vars::no_castle){
                if (legal_castle==chess_vars::q_castle){
                    allowed_moves.push_back( position(3,current_position.y())); // Queen side
                    destinations[position(3,current_position.y())].push_back(this);
                } else if (legal_castle==chess_vars::k_castle){
                    allowed_moves.push_back( position(7,current_position.y())); // King side
                    destinations[position(7,current_position.y())].push_back(this);
                } else {
                    allowed_moves.push_back( position(3,current_position.y())); // Queen side
                    allowed_moves.push_back( position(7,current_position.y())); // King side
                    destinations[position(3,current_position.y())].push_back(this);
                    destinations[position(7,current_position.y())].push_back(this);

                }
            }
            //TS: std::cout<<"\tPerforming a checkmate verification on: "<<color_to_char(owner)<<" "<<piece_to_char(abbreviation)<<std::endl;
            // TS: NOT NECESSARY : checked elsewhere
            // Update status of king
            //status = this->is_checkmated(); 
#ifdef DEBUGMODE
            std::cout<<"Number of king moves: "<<allowed_moves.size()<<std::endl;
            this -> print_allowed_moves();
#endif
            //TS: std::cout<<"\tExiting king..."<<std::endl;
        }
        void generate_threats()
        {
            std::vector<position>::iterator inc_begin {increments.begin()};
            std::vector<position>::iterator inc_end {increments.end()};
            std::vector<position>::iterator inc_it {};
            
            for (inc_it=inc_begin; inc_it<inc_end; ++inc_it){
                position increment{*inc_it};
                if ( (current_position+increment).is_valid()){
                    if (occupied_squares.count(current_position+increment)==0){
                        threats[owner].push_back(current_position+increment);
                    } else if (occupied_squares.at(current_position+increment)->get_owner()==owner){
                        defences[owner].push_back(current_position+increment);
                    } else{
                        threats[owner].push_back(current_position+increment);
                    }//TS: could keep the second if statement and append all other cases to threats
                }
            } 
        }

        chess_vars::castle can_castle()
        {
            if (has_castled){
                return chess_vars::no_castle;
            }

            bool q_side_legal{true}, k_side_legal{true};
            // For the castling to be legal:
            // - both rook and king must have not moved since start of game
            // - the spaces between the two must be empty
            // - the king must not pass through any squares threatened by their opponent.
            chess_vars::player_color opponent; 
            int back_rank;            
            if (owner==chess_vars::white){
                back_rank = 1;
                opponent = chess_vars::black;
            } else {
                back_rank = 8;
                opponent = chess_vars::white;
            }

            bool test_1, test_2, test_3, test_4, test_5, test_6;
            bool space_1, space_2, space_3, threat_1, threat_2;
            // King side
            if (occupied_squares.count(position(5,back_rank)) && occupied_squares.count(position(8,back_rank))){
                test_1 = occupied_squares.at(position(5,back_rank))->get_owner()==owner;
                test_2 = occupied_squares.at(position(5,back_rank))->get_abbrev()==chess_vars::king;
                test_3 = occupied_squares.at(position(5,back_rank))->check_if_moved()==false;
                test_4 = occupied_squares.at(position(8,back_rank))->get_owner()==owner;
                test_5 = occupied_squares.at(position(8,back_rank))->get_abbrev()==chess_vars::rook;
                test_6 = occupied_squares.at(position(8,back_rank))->check_if_moved()==false;
                // Now check empty spaces and threatened spaces
                space_1 = occupied_squares.count(position(6, back_rank))==0;
                space_2 = occupied_squares.count(position(7, back_rank))==0;    
                threat_1 = is_in(threats[opponent], position(6, back_rank));
                threat_2 = is_in(threats[opponent], position(7, back_rank));

                if ( ! (test_1 && test_2 && test_3 && test_4 && test_5 && test_6 )){
                    k_side_legal = false;
                    // reason: a piece has moved
                } else if ( ! (space_1 && space_2)){
                    k_side_legal = false;
                    // reason: a space is not empty
                } else if ( threat_1 || threat_2){
                    k_side_legal = false;
                    // reason: A square is threatened
                } else {
                    k_side_legal = true;
                }
                
            } else {
                k_side_legal = false;
            }

            // Queen side
            if (occupied_squares.count(position(5,back_rank)) && occupied_squares.count(position(8,back_rank))){
                test_1 = occupied_squares.at(position(5,back_rank))->get_owner()==owner;
                test_2 = occupied_squares.at(position(5,back_rank))->get_abbrev()==chess_vars::king;
                test_3 = occupied_squares.at(position(5,back_rank))->check_if_moved()==false;
                test_4 = occupied_squares.at(position(1,back_rank))->get_owner()==owner;
                test_5 = occupied_squares.at(position(1,back_rank))->get_abbrev()==chess_vars::rook;
                test_6 = occupied_squares.at(position(1,back_rank))->check_if_moved()==false;
                // Now check empty spaces and threatened spaces
                space_1 = occupied_squares.count(position(2, back_rank))==0;
                space_2 = occupied_squares.count(position(3, back_rank))==0;    
                space_3 = occupied_squares.count(position(4, back_rank))==0;    
                threat_1 = is_in(threats[opponent], position(3, back_rank));
                threat_2 = is_in(threats[opponent], position(4, back_rank));

                if ( ! (test_1 && test_2 && test_3 && test_4 && test_5 && test_6 )){
                    q_side_legal = false;
                    // reason: a piece has moved
                } else if ( ! (space_1 && space_2 && space_3)){
                    q_side_legal = false;
                    // reason: a space is not empty
                } else if ( threat_1 || threat_2){
                    q_side_legal = false;
                    // reason: A square is threatened
                }
                
            } else {
                q_side_legal = false;
            }

            // Return correct value:
            if (q_side_legal && k_side_legal){
                return chess_vars::both_castle;
            } else if (q_side_legal){
                return chess_vars::q_castle;
            } else if (k_side_legal){
                return chess_vars::k_castle;
            } else {
                return chess_vars::no_castle;
            }
        }

        bool is_checked(position new_pos = position(0,0))
        {
            //If not in check: return 0
            //Else: if can defend against check: return 1
            //  - are there non occupied + non threatened squares in allowed moves?
            //  - can a move of another piece capture the checking threat?
            //  - can a move of another piece defend against the check? 
            //Else can't defend: return 2
            position position_to_check;
            if (new_pos.is_valid() && !(new_pos==position(0,0))){
                position_to_check = new_pos;
            } else {
                position_to_check = current_position;
            }

            chess_vars::player_color opponent; 
            if (owner==chess_vars::white){
                opponent = chess_vars::black;
            } else {
                opponent = chess_vars::white;
            }
            if ( is_in(threats[opponent], position_to_check) ){
                return true; 
            } else {
                return false;
            }
            
            // Use reveal_check() in combination with verifying if knights nearby?
            return 0;
        }

        bool revealed_check(bool check_all = false) const // Check if a non-king move would reveal a check
        {
            // Check if line of sight to bishop/rook/queen
            std::vector<position> straights { position(0,1), position(0,-1), position(1,0), position(-1,0)};
            std::vector<position> diagonals { position(1,-1), position(-1,-1), position(-1,1), position(1,1)};
            bool test_opponent, test_rook, test_bishop, test_queen;
            // Check straights
            for ( auto it{ straights.begin()}; it < straights.end(); ++it){
                position line_of_sight {current_position + (*it)};
                while (line_of_sight.is_valid()){
                    if (occupied_squares.count(line_of_sight)){
                        test_opponent = occupied_squares.at(line_of_sight)->get_owner()!=owner;
                        test_rook = occupied_squares.at(line_of_sight)->get_abbrev()==chess_vars::rook;
                        test_queen = occupied_squares.at(line_of_sight)->get_abbrev()==chess_vars::queen;
                        if (test_opponent && (test_rook || test_queen)){
                            return true;                            
                        }
                        break; // No need to look further in this line 
                    }
                    line_of_sight = line_of_sight + (*it);
                }
            }
            // Check diagonals
            for ( auto it{ diagonals.begin()}; it < diagonals.end(); ++it){
                position line_of_sight {current_position + (*it)};
                while (line_of_sight.is_valid()){
                    if (occupied_squares.count(line_of_sight)){
                        test_opponent = occupied_squares.at(line_of_sight)->get_owner()!=owner;
                        test_bishop = occupied_squares.at(line_of_sight)->get_abbrev()==chess_vars::bishop;
                        test_queen = occupied_squares.at(line_of_sight)->get_abbrev()==chess_vars::queen;
                        if (test_opponent && (test_bishop || test_queen)){
                            return true;                            
                        }
                        break; // No need to look further in this line 
                    }
                    line_of_sight = line_of_sight + (*it);
                }
            }

            // Exit checks if not required
            if (!check_all){
                return false;
            }
            // Check knights. Note: a move can't reveal a check with a knight, but it is a useful functionality when checking for checkmate/stalemate.
            std::vector<position> knight_threats  { position(1,2), position(1,-2), position(-1,2), position(-1,-2),
                position(2,1), position(2,-1), position(-2,1), position(-2,-1)};
            for ( auto pos_it=knight_threats.begin(); pos_it<knight_threats.end(); ++pos_it){
                if (occupied_squares.count(current_position + (*pos_it))){
                    if ( occupied_squares.at(current_position + (*pos_it))->get_owner()!=owner && occupied_squares.at(current_position + (*pos_it))->get_abbrev()==chess_vars::knight){
                        return true;
                    }
                }
            }
            // Check pawns. Again, only useful when checking for checkmate/stalemate
            int direction{1};
            if (owner==chess_vars::black){
                direction = -1;
            }
            position pawn_pos;
            for (int diag{}; diag<2; diag++){
                pawn_pos.set( current_position.x()-1 + 2*diag, current_position.y()+direction);
                if (occupied_squares.count(pawn_pos) && pawn_pos.is_valid()){
                    if (occupied_squares.at(pawn_pos)->get_owner()!=owner && occupied_squares.at(pawn_pos)->get_abbrev()==chess_vars::pawn ){
                        return true;
                    }
                }
            }

            // If no lines of sight towards threatening piece: return false
            return false;

        }
        chess_vars::check_status is_checkmated()
        {
            //TS: std::cout<<"#Checkmate check for: "<<color_to_char(owner)<<" "<<piece_to_char(abbreviation)<<std::endl;
            is_in_check = this->is_checked();
            //TS: std::cout<<"\tCheck status: "<<is_in_check<<std::endl;

            bool allowed_moves_available {false};
            bool legal_moves_available {false};
            // Check first if allowed moves available
            for (std::map<position, piece*>::iterator iter= occupied_squares.begin(); iter!= occupied_squares.end(); ++iter){
                piece* temp {iter->second};
                if ( temp->get_owner()==owner && temp->get_number_of_moves()>0){                    
                    allowed_moves_available = true;
                    break;
                }
            }
            //TS: std::cout<<"\tAllowed moves: "<<allowed_moves_available<<std::endl;

            // First check for stalemate condition;
            if (!is_in_check){
                if (!allowed_moves_available){
                    return chess_vars::stalemate;
                }
            }

            // Now for each available move: see if there is at least one legal move 
            //TS: std::cout<<"\tChecking for at least one legal move: "<<allowed_moves_available<<std::endl;

            position old_position;
            std::pair<bool, piece*> captured_piece_state;
            for (std::map<position, piece*>::iterator iter= occupied_squares.begin(); iter!= occupied_squares.end(); ++iter){
                //TS: std::cout<<"\t-> Pos: "<<(iter->first)<<std::endl;
                if (!iter->first.is_valid()){
                    std::cerr<<"WARNING: skipping a piece in a non-valid position at "<<(iter->first)<<std::endl;    
                    //exit(EXIT_FAILURE);
                    continue;
                } else {
                    piece* temp {iter->second};
                    //TS: std::cout<<"\t\tTesting moves of: (pos: "<<(iter->first)<<"): "<<color_to_char(temp->get_owner())<<" "<< piece_to_char(temp->get_abbrev())<<std::endl;
                    if ( temp->get_owner()==owner && temp->get_number_of_moves()>0){                    
                        old_position = temp->location();
                        //TS: std::cout<<"\t\t--> Entered test loop"<<std::endl;
                        for (auto move_it = temp->get_allowed_iterators().first; move_it<temp->get_allowed_iterators().second; ++move_it){
                            if ( occupied_squares.count(*move_it) ){
                                if (occupied_squares.at(*move_it)->get_owner()==owner){
                                    continue;
                                }
                            }
                            captured_piece_state = temp->move( (*move_it));
                            legal_moves_available = !(this->revealed_check(true));
                            temp -> unmove(old_position, captured_piece_state);
                            
                            if (legal_moves_available){ // If at least one legal move found: can exit
                                break;
                            } 
                        }
                        //TS: std::cout<<"\t\t--> Exited test loop"<<std::endl;
                    }
                }
                if (legal_moves_available){ // Can exit second loop if at least one move already found
                    break; // TS: do we want to have a list of all legal moves?
                }
            }
            //TS: std::cout<<"\tLegal moves status: "<<legal_moves_available<<std::endl;

    
            //          |   check   | no check |
            //          |___________|__________|
            // moves    |   check   |   nominal|  00 01
            // no moves | checkmate | stalemate|  10 11


            if (is_in_check && legal_moves_available){
                return chess_vars::check;
            } else if (is_in_check && !legal_moves_available) {
                return chess_vars::checkmate;
            } else if (!is_in_check && !legal_moves_available){
                return chess_vars::stalemate;
            } else {
                return chess_vars::nominal;
            }
        }
        bool is_stalemate(){return false;}

};
