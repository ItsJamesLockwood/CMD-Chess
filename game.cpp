// This is the heart of the C++ Chess Project.
// This file contains all methods relating to:
// - interpretation of algebraic chess notation
// - reacting to requested moves: moving, promoting, capturing,...
// - verifies if checkmate
// - ...


#include<fstream>

#include "pieces.h"
#include "pieces.cpp"

#include "board.h"
#include "game.h"
#include "utils.cpp"

#pragma once


void chess::ask_game_option()
{
    bool valid_option{false};
    is_ready_status = false;
    want_to_resume = false;
    initialisation_requested = false;
    std::vector<std::string> input_options {"1","2","3","load","settings","game","change","quit","reset"};
    std::string input_msg{ "Please provide a game option: PvP (1), PvComputer (2), CvComputer (3), Resume current (G)ame, (L)oad Game, (C)hange (S)ettings, (Q)uit, (R)eset game"  };
    std::string error_msg{ "Invalid input. Choose an game option: "};
    
    // Get player to give a valid menu option
    char answer { ask_user_word(input_msg, error_msg, input_options)};

    switch (answer)
    {
    case '1':
        current_option = chess_vars::p_v_p;
        break;
    case '2':
        // Initialise one person and one computer player
        current_option = chess_vars::p_v_computer;

        // WARNING: Not currently cupported
        std::cout<< "PvC not currently supported. Defaulting to PvP..."<<std::endl;
        current_option = chess_vars::p_v_p;
        break;
    case '3':
        // Initialise two computer players
        current_option = chess_vars::c_v_computer;

        // WARNING: Not currently cupported
        std::cout<< "CvC not currently supported. Defaulting to PvP..."<<std::endl;
        current_option = chess_vars::p_v_p;

        break;
    case 'g':
        // Resume current game: skip initialisation and loading
        if (current_status==chess_vars::game_over){
            want_to_resume = false;
            current_status = chess_vars::game_on;
            // Let user know that no game is in progress
            return;
        } else {
            want_to_resume = is_ready_status = true;
            return;
        }
        break;
    case 'l': // Try to load a savefile
        current_option = chess_vars::load_game;
        try
        {
            chess::load_game();
            setup_type = chess_vars::loaded_board;
            initialisation_requested = true;
            current_status = chess_vars::game_on;
        }
        catch(const ChessException& e) // If any chess exceptions occur during loading: default here
        {
            std::cerr << e.what() << '\n';
            piece::reset_occupied_spaces();
            setup_type = chess_vars::default_board;    
            initialisation_requested = false;
            current_status = chess_vars::game_over;
        }
        return;
        break;
    case 's':
    case 'c': // Not currently implemented: original aim was to change the board-printing preferences
        current_option = chess_vars::change_settings;
        return;
        break;
    case 'q':
        current_option = chess_vars::quit_program;
        std::cout<<"Thank you for playing Chess with me!"<<std::endl; 
        exit(EXIT_SUCCESS);
        //Note: does not currently confirm with user whether to exit program
        break;
    case 'r':
        // Reset loaded game options, etc.
        piece::reset_occupied_spaces();
        setup_type = chess_vars::default_board;
        current_status = chess_vars::game_over;
        return;
        // Other reset functions go here
        break;
    default:
        // Should not be accessible if the input_options were declared correctly.
        break;
    }
    std::cout<<"Game option: "<<current_option<<std::endl;   
    

    // If player starting a new game: choose color
    std::vector<std::string> player_options {"w","b","quit"};
    std::string color_msg { "Choose a color:  (W)hite / (B)lack : " };   
    std::string color_not_found_msg{"Invalid input. Choose a player (W/B): "};
    char player_ans { ask_user_word(color_msg, color_not_found_msg, player_options)};
    switch (player_ans)
    {
    case 'w':
        main_player = chess_vars::white;
        chess_board.change_main_player("white");
        break;
    case 'b':
        main_player = chess_vars::black;
        chess_board.change_main_player("black");
        break;
    default:
        // Should not be necessary
        break;
    }

    // Set variables to GO for new game
    std::cout<<"Chosen player color: "<<player_ans<<" "<<main_player<<std::endl;
    is_ready_status = true;
    current_status = chess_vars::game_on;
    initialisation_requested = true;
}


void chess::initialise_game()
{
    // When PvC and CvC are implemented: Do we initialise players here too?

    // Initialise board
    if (setup_type == chess_vars::loaded_board){
        //TS: dummy var
        chess_board.load_board((*occupied),(*the_kings));// !! Need to also load the kings and main player
    } else {
        piece::reset_occupied_spaces();
        chess_board.initialise_board();
        chess_board.print_board();
        the_kings = &chess_board.get_the_kings();
        occupied = piece::get_locations();
    }

    // Temporarily switch to opponent to generate threats
    current_player = switch_player(current_player);
    this -> generate_threats();
    // Switch back to initial player to generate allowed moves
    current_player = switch_player(current_player);    
    this -> generate_moves();
}

bool chess::over()
{
    if (current_status == chess_vars::game_over){
        return true;
    } else {
        return false;
    }
}
bool chess::is_ready()
{
    return is_ready_status;
}
bool chess::resume_game()
{
    return want_to_resume;
}
bool chess::want_initialisation()
{
    return initialisation_requested;
}

// Generate moves for all the pieces owned by the current player
void chess::generate_moves()
{
    //Options:
    // 1: vector of all pieces. Con: will need to iterate through to find which one to delete upon capture
    // 2: iterate through occupied spaces. Pro: no need to update yet another container. Con: need to check all the squares. EDIT: actually dont need to check positions, just iterate through keys 

    //Option 2:
    //First we delete the destinations entirely. 
    (*accessible_squares).clear();
    for (std::map<position, piece*>::iterator iter= (*occupied).begin(); iter!=(*occupied).end(); ++iter){
        //TS: std::cout<<"--> Generating moves for "<<piece_to_char(iter->second->get_abbrev())<<std::endl;
        piece* temp {iter->second};
        if (temp->get_owner()!=current_player){
            continue; // Don't generate moves for pieces which don't belong to me
        }
        
        (*temp).generate_allowed_moves();
    } 
}

// If pawn promotion: require initialisation of a new dynamically allocated piece pointer
piece *promote_piece(chess_vars::player_color color, position location, chess_vars::piece_type promotion_type)
{
    piece* new_piece;
    switch (promotion_type)
    {
    case chess_vars::queen:
        new_piece = new queen(color, chess_vars::queen, location);  
        new_piece->set_moved(true);     
        return new_piece;
    case chess_vars::rook:
        new_piece = new rook(color, chess_vars::rook, location);  
        new_piece->set_moved(true);     
        return new_piece;
    case chess_vars::bishop:
        new_piece = new bishop(color, chess_vars::bishop, location);  
        new_piece->set_moved(true);     
        return new_piece;
    case chess_vars::knight:
        new_piece = new knight(color, chess_vars::knight, location);  
        new_piece->set_moved(true);     
        return new_piece;
    // chess_vars::king:
    // chess_vars::pawn:
    // chess_vars::nancy_rothwell:
    default: //Coding style: does it make sense to have pieces as well as default
        std::cerr<<"WARNING: tried to promote a pawn to a "<<piece_to_char(promotion_type)<<". Defaulting to a queen..."<<std::endl;
        new_piece = new queen(color, chess_vars::queen, location);  
        new_piece->set_moved(true);     
        return new_piece;
    }
}

// Given a 2-length string: convert algebraic position to numerical coordinates
position convert_to_pos(std::string request)
{
    if (request.length()!=2){
        // Throw error: this is not meant to happen
        return position(0,0);
    }
    std::vector<char> files{'a','b','c','d','e','f','g','h'};
    std::vector<char> ranks{'1','2','3','4','5','6','7','8'};
    position new_position;

    if (is_in(files,request[0]) && is_in(ranks,request[1])){
        int x_coord { get_index(files, request[0]) +1 };
        int y_coord { get_index(ranks, request[1]) +1 };
        new_position.set(x_coord,y_coord);
        return new_position;
    } else{
        return position(0,0);
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%% MOVE INTERPRETATION %%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


// Hefty piece of code to interpret an algebraic request: e.g. translates 'axb' to chess language to capture pawn on file b with pawn on file a
move_request interpret_move(std::string request)
{
    // If request does not contain the letter b: can be lowered (else: confusion between bishop and file b) 
    if (!char_in_str(request,'b') && !char_in_str(request, 'B')){
        to_lower(request);
    } // else : do nothing
    
    // Check if request was not actually a move: simple outcomes which require little interpretation
    std::vector<std::string> non_move_requests {"save", "draw", "ignore draw","resign","quit","menu"}; //Not sure if quit should be left in there
    move_request move;
    bool non_move;
    if ( request.length()>1){
        non_move = is_in(non_move_requests, request); 
    } else {
        non_move = is_in_char(non_move_requests, request[0]);
    }

    // Process all non-moves here and exit function: no further translation required
    if (non_move){
        move.valid = true;
        switch (request[0]) // Note: we have already checked that a move such as Re4 does not trigger the resignation.
        {
        case 's':
            // Save game
            move.type = chess_vars::save;
            return move;
            break;
        case 'd':
            // Offer a draw
            move.type = chess_vars::offer_draw;
            return move;
            break;
        case 'i':
            // Ignore/rescing draw
            move.type = chess_vars::remove_draw;
            return move;
            break;
        case 'r':
        case 'q':
            // Resign the game
            // Are you sure you want to resign?
            move.type = chess_vars::resign;
            return move;
            break;
        case 'm':
            // Goto menu
            // Save current game 
            move.type = chess_vars::menu;
            return move;
            break;
        default: // Should not be accessible
            move.valid = false;
            break;
        }
    }

    std::vector<char> files{'a','b','c','d','e','f','g','h'};
    std::vector<char> ranks{'1','2','3','4','5','6','7','8'};
    std::vector<char> abbrevs{'R','N','B','K','Q','r','n','k','q'}; //Note: bishop must be provided in uppercase, else it will not be recognised
    position new_position;
    // Step 0: check for hanging characters (i.e. draw offer: "=")
    // NOTE: draw offers not currently implemented: this will currently not affect the game
    if (request.length()>2){
        if (request.back()=='='){
            request = request.substr(0, request.size()-1);    
            move.draw_offer = true;
        }
    } 

    // Assume move is invalid, and change if not
    move.valid = false;
    move.type = chess_vars::move;

    // Check first for simple move request (2-move)
    if (request.length()==2){
        //First option: king castle
        if (request == "oo"){
            move.k_castle = true;
            move.valid = true;
            move.end = position(7,0);
            return move;
        } else { // Else: must be an unambiguous end-position
            new_position = convert_to_pos(request);
            if (!new_position.is_valid()){
                std::cerr<<"At interpretation: single square was invalid!"<<std::endl;
                return move;
            }
            move.pawn_move = true;
            move.id = chess_vars::nancy_rothwell; // Non-specific move
            move.end = new_position;
        }

    // Next: check if the move is invalid
    } else if (request.length()<2){
        std::cerr<<"At interpretation: move request was too short!"<<std::endl;
        return move;
    // Next: check moves of length 3 to 6 (longest conceivable move)
    } else if (request.length()==3){
        if (request == "o-o"){ // Kingside castle
            move.k_castle = true;
            move.valid = true;
            move.end = position(7,0);
            move.type = chess_vars::move;
            return move;
        } else if (request == "ooo"){ // Queenside castle
            move.q_castle = true;
            move.valid = true;
            move.end = position(3,0);
            move.type = chess_vars::move;
            return move;
        } else if (request[1] == 'x'){ // Pawn capture
            if (!is_in(files,request[0]) || !is_in(files,request[2])){
                std::cerr<<"At interpretation: did not recognise a valid pawn capture format."<<std::endl;
                return move;
            }
            move.pawn_attack = true;
            move.pawn_move = true;
            int start { get_index(files, request[0]) +1};
            int end { get_index(files, request[2]) +1};
            move.start = position(start,0);
            move.end = position(end,0);
            move.id = chess_vars::pawn;
        } else if (is_in(abbrevs, request[2])){ // Pawn promotion
            if (request[2]=='k'){
                std::cerr<<"At interpretation: Cannot promote a pawn to a king"<<std::endl;
                move.valid = false;
                return move;
            }
            new_position = convert_to_pos(request.substr(0,2));
            if ( !new_position.is_valid() ||(new_position.y()!=8 && new_position.y()!=1)){ // Invalid promotion if pawn move not to 8th rank
                std::cerr<<"At interpretation: Move to promote pawn was invalid or not to the last rank."<<std::endl;
                return move;
            }
            move.pawn_move = true;
            move.end = new_position;
            move.start = position(move.end.x(), 0);
            move.id = char_to_piece(request[2]);
            move.promotion = true;
        } else if (is_in(abbrevs, request[0])){ //Non-unique move: has a piece specifier (e.g. Q, B, K, R, N)
            new_position = convert_to_pos(request.substr(1,2));
            if (!new_position.is_valid()){
                std::cerr<<"At interpretation: square with single identifier was not valid!"<<std::endl;
                return move;
            }   
            move.end = new_position;
            move.id = char_to_piece(request[0]);
        } else { // No other moves supported
            std::cerr<<"At interpretation: the 3-request was not recognised as a valid move!"<<std::endl;
            return move;
        } 
    } else if (request.length()==4 ){ 
        //TS Note: could include the request[0] check up here (streamline) 
        if (request[1]=='x' && is_in(abbrevs, request[0])){ // Standard capture with a piece specifier
            new_position = convert_to_pos(request.substr(2,2));
            if (!new_position.is_valid()){
                std::cerr<<"At interpretation: end square for standard capture was not recognised (4-request)."<<std::endl;
                return move;
            }
            move.end = new_position;
            move.capture = true;
            move.id = char_to_piece(request[0]);
        } else if (request[1]=='x' && is_in(files, request[0]) && is_in(files,request[2]) && is_in(files,request[3])){ // Pawn capture to a promotion
            if (request[3]=='k'){ // Can't promote pawn to king
                std::cerr<<"ERROR: cannot promote piece to king"<<std::endl;
                move.valid = false;
                return move;
            }
            move.pawn_attack = true;
            move.pawn_move = true;
            int start { get_index(files, request[0]) +1};
            int end { get_index(files, request[2]) +1};
            move.start = position(start,0);
            move.end = position(end,0);
            move.id = char_to_piece(request[3]);
            move.promotion = true;
        } else if (request[1]=='x' && is_in(files, request[0])){ // Ambiguous pawn capture: e.g. axb
            new_position = convert_to_pos(request.substr(2,2));
            int old_x { get_index(files,request[0]) +1};
            if (!new_position.is_valid() || abs(old_x - new_position.x())!=1 ){
                std::cerr<<"At interpretation: while disambiguating pawn capture, move was not recognised as a valid pawn move."<<std::endl;
                return move;
            }
            move.start = position( old_x, 0);
            move.end = new_position;
            move.id = chess_vars::pawn;
            move.pawn_move = true;
            move.capture = true;
        } else if (is_in(files, request[1]) && is_in(abbrevs, request[0])){ // Disambiguate piece with rank
            int file { get_index(files, request[1]) +1};
            if (file>8 || file <1){
                std::cerr<<"At interpretation: disambiguating file ras not recognised."<<std::endl;
                return move;
            }
            move.start = position(file,0);
            new_position = convert_to_pos(request.substr(2,2));
            if (!new_position.is_valid()){
                std::cerr<<"At interpretation: end square was not recognised (4-request)."<<std::endl;
                return move;
            }
            move.id = char_to_piece(request[0]);
            move.end = new_position;

        } else if (is_in(ranks,request[1]) && is_in(abbrevs, request[0])){ // Disambiguate piece with file
            int rank { get_index(ranks, request[1]) +1};
            if (rank>8 || rank <1){
                std::cerr<<"At interpretation: disambiguating rank was not recognised."<<std::endl;
                return move;
            }
            move.start = position(0,rank);
            new_position = convert_to_pos(request.substr(2,2));
            if (!new_position.is_valid()){
                std::cerr<<"At interpretation: end square was not recognised (4-request with rank)."<<std::endl;
                return move;
            }
            move.id = char_to_piece(request[0]);
            move.end = new_position;
        } else { // No other 4-char moves supported
            std::cerr<<"At interpretation: the 4-request was not recognised as a valid request."<<std::endl;
            return move;
        }
    } else if (request.length() == 5){
        if (request == "o-o-o"){ //Queenside castle
            move.q_castle = true;
            move.valid = true;
            move.end = position(3,0);
            move.type = chess_vars::move;
            return move;
        } else if (request[2]=='x'){ // Non-unique piece and rank/file capture
            bool test_0, test_1a, test_1b, test_3, test_4;
            test_0 = is_in(abbrevs, request[0]);
            test_1a = is_in(files, request[1]);
            test_1b = is_in(ranks,request[1]);
            test_3 = is_in(files, request[3]);
            test_4 = is_in(ranks, request[4]);
            if ( ! (test_0 &&  (test_1a || test_1b)  && test_3 && test_4)){
                std::cerr<<"At interpretation: disambiguating rank or file capture did not meet all the tests."<<std::endl;
                return move;
            }
            position start, end;
            if (test_1a){
                int file { get_index(files, request[1]) +1};
                start = position(0,file);
                if (file>8 || file <1){
                    std::cerr<<"At interpretation: disambiguating file invalid (5-request)."<<std::endl;
                    return move;
                }
                move.start = start;
            } else {
                int rank { get_index(ranks, request[1]) +1};
                start = position(rank,0);
                if (rank>8 || rank <1){
                    std::cerr<<"At interpretation: disambiguating rank invalid (5-request)."<<std::endl;
                    return move;
                }
                move.start = start;
            }
            end = convert_to_pos(request.substr(3,2));
            if (!end.is_valid()){
                std::cerr<<"At interpretation: end move of capture was not recognised (5-request)."<<std::endl;
                return move;
            }
            move.end = end;
            move.id = char_to_piece(request[0]);
        } else {    // Non-unique piece with start and end position
            bool test_0, test_1, test_2, test_3, test_4;
            test_0 = is_in(abbrevs, request[0]);
            test_1 = is_in(files, request[1]);
            test_2 = is_in(ranks, request[2]);
            test_3 = is_in(files, request[3]);
            test_4 = is_in(ranks, request[4]);
            if ( ! (test_0 &&  test_1 && test_2 && test_3 && test_4)){
                std::cerr<<"At interpretation: request with piece, start and end squares did not meet all the tests (5-request)."<<std::endl;
                return move;
            }
            position pos_start, pos_end;
            pos_start = convert_to_pos(request.substr(1,2));
            pos_end = convert_to_pos(request.substr(3,2));
            if (! (pos_end.is_valid() && pos_start.is_valid())){
                std::cerr<<"At interpretation: request with both start and end position had an invalid position (5-request)."<<std::endl;
                return move;
            }
            move.start = pos_start;
            move.end = pos_end;
            move.id = char_to_piece(request[0]);
        }
    } else if (request.length() == 6){ // Currently only accepting disambiguating captures
        bool test_0, test_1, test_2, test_3, test_4, test_5;
        test_0 = is_in(abbrevs, request[0]);
        test_1 = is_in(files, request[1]);
        test_2 = is_in(ranks, request[2]);
        test_3 = (request[3] == 'x');
        test_4 = is_in(files, request[4]);
        test_5 = is_in(ranks, request[5]);
        if ( ! (test_0 &&  test_1 && test_2 && test_3 && test_4 && test_5)){
            std::cerr<<"At interpretation: 6-reqest does not meet all the tests."<<std::endl;
            return move;
        }
        position pos_start, pos_end;
        pos_start = convert_to_pos(request.substr(1,2));
        pos_end = convert_to_pos(request.substr(4,2));
        if (! (pos_end.is_valid() && pos_start.is_valid())){
            std::cerr<<"At interpretation: start or end position of 6-request is invalid."<<std::endl;
            return move;
        }
        move.start = pos_start;
        move.end = pos_end;
        move.id = char_to_piece(request[0]);
        move.capture = true;
    } else { // No 6-char moves supported 
        //TS: could trim end/beginning of request to see if valid move found
        std::cerr<<"At interpretation: requests of length 7 or greater not currently supported!"<<std::endl;
        return move;
    }    
    move.type = chess_vars::move;
    move.valid = true;
    return move;
} 
// Summary of allowed move:
// Thinking: 1 simple notation, can complicate later
// 2: destination
// -kingside castle (OO)(00)
// 3:
// - dest + = DONE
// - pawn promotion DONE
// - piece + dest DONE
// - kingside castle (O-O) DONE
// - queenside castle (OOO) DONE
// - pawn capture (exd) DONE
// 4:
// - piece + capture + dest DONE
// - piece (non-unique) + rank/file + dest  DONE
// 5: 
// - piece (non-unique) + rank + file + dest #Rare
// - queenside castle (O-O-O) DONE
// 6:
// - piece (non-unique) + rank-file + capture + dest #Rare

// +1:
// - draw offer: '='


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%% Convert request to move %%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Takes the move_request generated by interpret_move and enacts the request if it is valid and legal
void chess::ask_for_move()
{
    bool valid_move{false};
    bool leaves_check{true};
    move_request move;
    move.valid = false;
    piece *moving_piece;
    piece *castling_rook;
    
#ifdef DEBUGMODE
    std::cout<<"ALLOWED MOVES:"<<std::endl;
    this->print_accessible_squares();
#endif
    std::string optional_msg { "Select one of the following, or provide a (list of) move: (D)raw, (U)ndraw, (R)esgin, (M)enu, (S)ave: "};
    // Keep asking for a new move if last move left a check or was invalid
    while ( leaves_check || !move.valid){
            // Debugging statements:
                //std::cout<<"Premoves status: "<<premoves.count(current_player)<<"\n";
                //std::cout<<"Premoves size: "<<premoves[current_player].size()<<"\n";

            // Give player option to load in premoves
            while (premoves[current_player].empty()){
                premoves[current_player] = user_load_queue(premoves[current_player], optional_msg);
            }

            //Note: will lead to error if premoves is empty
            //Note: add error checking to ensure not trying to remove element from empty queue
            if (premoves[current_player].empty()){
                std::cerr<<"ERROR: empty queue!!"<<std::endl;
                throw EmptyQueueException();
            }

            // Pull the first request from the premoves deque
            std::string request { premoves[current_player].front() };
            std::cout<<"Request: "<<request<<std::endl;
            if (request=="Bd4"){
                std::cout<<"Press enter to continue:";
                char temp_c;
                std::cin>>temp_c;
            }
            premoves[current_player].pop_front();

            // The magic begins: a move_request is generated from the string request.
            // This move_request is then applied to the current board status to ensure it is unambigous, valid, and legal.
            move_request move { interpret_move(request)} ;
            current_request = chess_vars::invalid_request;

            // If move is valid: then interpret
            if (move.valid){
                // If not a move: simply enact the non-move
                if (move.type != chess_vars::move){
                    // handle move : code for each sceanrio goes here
                    current_request = move.type;
                    switch(move.type){
                        case chess_vars::undo:
                            // Only possible against computer: revert the last two moves
                        case chess_vars::save:
                            // Save game to file. IF file exists already, allow user to modify?
                            this->save_game();
                            break;
                        case chess_vars::offer_draw:
                            // Allow opponent to accept draw offer. If draw offer already from opponent, end game. 
                            // If applied twice, rescind draw offer. Draw offer in algebraic notation takes precedence.
                            // NOT IMPLEMENTED IN CURRENT RELEASE
                        case chess_vars::remove_draw:   
                            // Rescind draw offer. Decline draw offer from opponent.
                            // Not necessary, as a simple move wll negate the offer.
                            // NOT IMPLEMENTED IN CURRENT RELEASE
                        case chess_vars::resign:
                            // Resign from game, opponent wins.
                            // NOT IMPLEMENTED IN CURRENT RELEASE
                        case chess_vars::menu:
                            // Make temporary save of game before exiting to the menu.
                            break;
                        default:
                            // If any other non-move requests are introduced they will default here.
                            current_request = chess_vars::invalid_request;
                            break;
                    }
                    // No need to go any further: request is valid and requires no further interpretation
                    if (current_request!= chess_vars::invalid_request) return;

                } else if (move.k_castle){ // Instead: check if (position(7,backrank)) is in destinations and that type is king
                    chess_vars::castle castle_type { (*the_kings)[current_player]->can_castle()};
                    int backrank{1};
                    if (current_player==chess_vars::black) backrank = 8;
                    if (castle_type==chess_vars::k_castle || castle_type==chess_vars::both_castle){
                        move.end = position(7, backrank);
                        move.start = position(5, backrank);
                        move.castle_end = position(6,backrank);
                        moving_piece = chess_board.get_locations()->at(move.start);
                        castling_rook = chess_board.get_locations()->at(position(8,backrank));
                    } else{
                        // not able to castle: ask again
                        move.valid = false;
                        std::cerr<<"At conversion: King castling not accepted."<<std::endl;
                    }
                } else if (move.q_castle){ // Asked for queen castling: check if possible
                    chess_vars::castle castle_type { (*the_kings)[current_player]->can_castle()};
                    int backrank{1};
                    if (current_player==chess_vars::black) backrank = 8;
                    if (castle_type==chess_vars::q_castle || castle_type==chess_vars::both_castle){
                        move.end = position(3, backrank);
                        move.start = position(5, backrank);
                        move.castle_end = position(4,backrank);
                        moving_piece = chess_board.get_locations()->at(move.start);
                        castling_rook = chess_board.get_locations()->at(position(1,backrank));
                    } else{
                        // not able to castle: ask again
                        move.valid = false;
                        std::cerr<<"At conversion: Queen castling not accepted."<<std::endl;
                    }
                } else if ( move.promotion){ // Pawn (capture to) promotion //TS : should work for both forms of promotion (move/capture)
                    int last_rank{1}, direction{-1};
                    if (current_player==chess_vars::white){
                        last_rank = 8;
                        direction = 1;
                    }
                    position start_position {position(move.start.x(), last_rank-direction)};
                    position end_position {position(move.end.x(), last_rank)};
                    if ( (*accessible_squares).count(move.end)){ // Check if destination has possible moves
                        bool test_1, test_2;
                        int found_one{};
                        for (auto it{(*accessible_squares).at(end_position).begin()}; it< (*accessible_squares).at(end_position).end(); ++it ){ // Check all squares in end file
                            test_1 = (*it)->get_owner() == current_player;
                            test_2 = (*it)->get_abbrev() == chess_vars::pawn;
                            
                            if (test_1 && test_2){
                                moving_piece = (*it);
                                found_one ++;
                                break;
                            }
                        }  
                        if (found_one==0){
                            std::cerr<<"At conversion: No pawns were found that could make this move."<<std::endl;
                            move.valid = false;
                        }
                    } else { // Destination has no moves. Pawn unable to move here.
                        std::cerr<<"At conversion: no piece can move to this position."<<std::endl;
                        move.valid = false;
                    }

                } else if (move.pawn_move && move.pawn_attack){ // Pawn capture (as ranks need to be deduced)
                    // Deduce positions
                    int found_one{0};
                    position end_position; // Bad practise to change a variable used in the for loop
                    for (int r{1}; r<=8; r++){
                        if ((*accessible_squares).count(position(move.end.x(), r))){
                            for (auto it{(*accessible_squares).at(position(move.end.x(),r)).begin()}; it< (*accessible_squares).at(position(move.end.x(),r)).end(); ++it ){ // Check all squares in end file
                                bool test_1, test_2, test_3;
                                // Note: we don't have to test if move is indeed a capture, as it must be so to be added to destinations;
                                
                                test_1 = (*it)->get_owner() == current_player; // If piece belongs to me: can move
                                test_2 = (*it)->get_abbrev() == chess_vars::pawn; // If piece is a pawn: can perform a pawn move
                                test_3 = (*it)->location().x() == move.start.x(); // Check that this piece has the right starting file
                                

                                if (test_1 && test_2 && test_3){
                                    if (found_one>0){
                                        // Non-unique: please specify: need a specific end square
                                        found_one ++;
                                        move.valid = false;
                                        std::cerr<<"The pawn capture was not unique. That's a rare situation! Please specify the end-square!"<<std::endl;
                                        break; // Second pawn found: exit both loops (EDIT: unless we want to tell user how many pawns are there)
                                    } else {
                                        found_one ++;
                                        moving_piece = (*it);
                                        end_position.set(move.end.x(), r);
                                        break; // Can proceed to next destination, as only one pawn can acess a square from a given file
                                    }
                                } // Else: not a valid pawn move: keep_looking
                            }
                            if (found_one>1){ // Need a second check to exit the second loop
                                break;
                            }

                        }
                    }
                    if ( found_one==0){ // No valid pieces found
                        // Invalid move: ask again
                        std::cerr<< "At conversion: no pawn capture found!"<<std::endl;
                        move.valid = false;
                    } else if (found_one==1){ // One and only one valid piece found!
                        move.end = end_position; 
                    }
                } else { // All other cases: give exact square reference 
                    if ((*accessible_squares).count(move.end)){
                        // Valid destination
                        // Now locate actual piece based on: start + id
                        // If start rank!=0: must be same
                        // If start file!=0: must be same
                        // If id!='': must be same
                        int options{}, pawn_options{},capturing_pawn_options{};
                        bool correct;
                        
                        for (auto it{ (*accessible_squares).at(move.end).begin() }; it < (*accessible_squares).at(move.end).end(); ++it){    
                            correct = true;
                            if ((*it)->get_owner()!=current_player){
                                continue;
                            }
                            if (move.start.x()!=0){
                                correct &= (*it)->location().x() == move.start.x();
                            }
                            if (move.start.y()!=0){
                                correct &= (*it)->location().y() == move.start.y();
                            }
                            //if (move.id != chess_vars::pawn){ //TS: what is this meant to achieve? Should I not be checking this no matter what?
                            if (move.id != chess_vars::nancy_rothwell){ //If type has purposefully not been set, don't bother checking the type of piece.
                                correct &= (*it)->get_abbrev() == move.id;
                            }

                            // Cases:
                            // 1: dealing with a non-pawn piece from move e.g. XXe7
                            // 2: dealing with a pawn which is moving forwards (only one possible piece, and the default)
                            // 3: dealing with a capturing pawn which was written incorrectly // No need to support this currently
                            if (correct && (*it)->get_abbrev()!=chess_vars::pawn && pawn_options==0 && capturing_pawn_options==0){ // Case 1: If no non-pawn found, keep track of piece.
                                moving_piece = (*it);
                                move.valid = true;
                                options ++;
                                std::cout<<"At search: found a "<<piece_to_char((*it)->get_abbrev())<<" at "<<(*it)->location()<<std::endl;
                            } else if (correct && (*it)->get_abbrev()==chess_vars::pawn ){ //&& (*it)->location().x()==move.end.x()){ // Case 2: If all tests correct, and piece is a (NON-capturing) pawn, takes preference over other moves, so long as only one available
                                if (pawn_options>1 || capturing_pawn_options>1){
                                    std::cerr<<"There are somehow multiple pawns able to access this same square: "<<move.end; // Something went terribly wrong: exiting!
                                    exit(EXIT_FAILURE);
                                }
                                moving_piece = (*it);
                                move.valid = true;
                                if (move.capture){
                                    pawn_options ++;
                                    std::cout<<"At search: found a "<<piece_to_char((*it)->get_abbrev())<<" at "<<(*it)->location()<<std::endl;
                                } else {
                                    capturing_pawn_options++;
                                }
                            } else { // Case 3: incorrect or an undefined piecd or a capturing pawn or a second valid non-pawn piece
                                // If capture and pawn_options = 0 -> take
                                // If capture and pawn_options > 0 -> ignore
                                // Skip and keep looking
                                //std::cout<<"Not yet supported..."<<std::endl;
                            }

                            if (options>1){
                                // Non-unique: ask for details
                                move.valid = false;
                                std::cerr<< "At conversion: non-unique end square. Please a further identifier!" <<std::endl;
                                break; // Optional: could check to see how many fit the brief
                            }
                        } 
                        if (options == 0 && pawn_options==0 && capturing_pawn_options==0){
                            // If piece invalid
                            move.valid = false;
                            std::cerr <<"At conversion: No pieces of yours were found which could move there!" <<std::endl;
                        }
                    } else { // No piece can be moved to this end position
                        // Invalid move: ask again
                        std::cerr<<"At conversion: No piece is able to move here!"<<std::endl;
                        move.valid = false;
                    }
                }
            } 

        // Now the rest of the magic happens: the move request has been fully fleshed out: it will now be used to move the exact piece requested  
        // Make move if provided  
        if (move.valid && move.end.is_valid()){ 
            try{
                // Make the move and check if it reveals a check.
                bool leads_to_check {this->make_move(move, moving_piece, castling_rook)};
                if (!leads_to_check){
                    leaves_check = false;
                    int backrank{1};
                    if (current_player==chess_vars::white){
                        backrank = 8;
                    } 
                    if (move.promotion){ // Deal with pawn promotions
                        // Delete piece in last rank + create a new piece in its place
                        if ((*occupied).at(move.end)->get_abbrev()==chess_vars::king){
                            throw KingDeletionException();
                        }
                        delete (*occupied).at(move.end);
                        (*occupied).erase(move.end);
                        piece* temp {promote_piece(current_player, move.end, move.id)};
                        (*occupied)[move.end] = temp;
                        std::cout<<"Check promotion ptr:"<<std::endl;
                        std::cout<<(*temp)<<std::endl;
                        std::cout<<"Check occupied space promotion:"<<std::endl;
                        std::cout<<*(*occupied).at(move.end)<<std::endl;
                    } else if (moving_piece->get_abbrev()==chess_vars::pawn && move.end.y()==backrank){ // Deal with unspecified pawn promotions
                        // Undeclared promotion. Resolve here.
                        std::string msg {"You've moved a pawn to the last rank. Choose what to promote it to: (Q)ueen, (R)ook, (B)ishop, k(N)ight, (E)xit: "};
                        std::string error_msg {"Not a recognised piece."};
                        std::vector<std::string> input_options {"q", "b", "r","n"};
                        char promotion { ask_user_word(msg,error_msg, input_options)};
                        chess_vars::piece_type promotion_type;
                        switch (promotion)
                        {
                        case 'e':
                        case 'q':
                            promotion_type = chess_vars::queen;
                            break;
                        case 'b':
                            promotion_type = chess_vars::bishop;
                            break;
                        case 'r':
                            promotion_type = chess_vars::rook;
                            break;
                        case 'n':
                            promotion_type = chess_vars::knight;
                            break;
                        default:
                            break;
                        }
                        if ((*occupied).at(move.end)->get_abbrev()==chess_vars::king){
                            //std::cerr<<"Asked to delete a king!"<<std::endl;
                            throw KingDeletionException();
                        }
                        delete (*occupied).at(move.end);
                        (*occupied).erase(move.end);
                        piece* temp {promote_piece(current_player, move.end, promotion_type)};
                        (*occupied)[move.end] = temp;
                    }
                    current_request = move.type;
                    break; // Technically sufficient to exit the loop
                } else{
                    std::cout<<"Move would lead to check!"<<std::endl;
                }       
            } catch (KingDeletionException& e){ // Ensure the King does not get deleted
                std::cerr<< e.what() << "\n";
                move.valid = false;
                current_request = chess_vars::invalid_request;
                return; // Exit function and attempt move from scratch  
            }
        } else if (!move.end.is_valid() && move.valid){ // Sanity check: should not be possible 
            std::cerr<<"ERROR: reaching end of function with an invalid end position but with the valid flag set to TRUE."<<std::endl;
        }
        // If move failed: let player know!
        std::cout<<"Welp! Move not recognised... Try again..."<<std::endl;
        
        // Clear premoves + start over
        premoves[current_player].clear();
            
        // Theoretically easy to check:
        // If not in check:
        // Is their a direct line to a bishop/rook/queen in the 8 directions?
        // i.e. don't need to check all pieces on all squares
        // If King move: check if moving into threatened position

        // If in check: move.valid= false
        // Else leaves check = true        
    }
    
}
//Summary: 
// If move:
// - if premove in queue: check if valid
//      - if premove not valid: ask for new move
//      - else if move leaves in check: ask for new move
//      - else: process move
// - if no moves in queue: ask for new move
// - if request if : undo, save, draw, resign, quit, menu: don't move and act
// - if multiple pieces available: ask for which one

//Prcess of making a move, checking for revealed check, backtracking if it does.
bool chess::make_move(move_request selected_move, piece* moving_piece, piece* castling_rook = nullptr)
{
    typedef std::pair<bool, piece*> bool_piece_pair;

    piece current_piece_state = *moving_piece;
    position old_position { moving_piece->location()};
    position old_rook_position;
    piece current_rook_state;
    bool_piece_pair captured_piece_state; 
    
    // First ensure king doesn't move into threatened position
    if (moving_piece->get_abbrev()==chess_vars::king){
        // Sanity check: should not be moving to an invalid position anyway
        if (!selected_move.end.is_valid() && !(selected_move.k_castle || selected_move.q_castle)){
            std::cerr<<"About to move to an unvalid position despite previous checks. Exiting..."<<std::endl;
            exit(EXIT_FAILURE);
        }
        if ( (*the_kings)[current_player]->is_checked(selected_move.end)) {
            // No need to roll back as the King will not have moved
            return true;
        }
    }
    // Next, ensure I'm not capturing my own piece before moving (Sanity check: should not have been allowed otherwise)
    // If capturing enemy piece: safeguard it first
    if ((*occupied).count(selected_move.end)){
        if ( (*occupied).at(selected_move.end)->get_owner()==current_player){
            // WARNING: trying to capture own piece. Return true
            std::cout << "WARNING: that position is occupied with my piece: "<<std::endl
                      << (*occupied).at(selected_move.end)
                      << "Try again..."<<std::endl;
            return true;
        } 
    }   

    // If a piece was captured: needs to be stored in case move is illegal
    captured_piece_state =  moving_piece->move(selected_move.end);
    if (selected_move.k_castle || selected_move.q_castle){
        current_rook_state = *castling_rook;
        old_rook_position = castling_rook ->location();
        castling_rook->move(selected_move.castle_end); // No capture intended: no need to store rvalue in an lvalue
    }

    // If the move reveals a check, roll back
    if ( (*the_kings)[current_player]->revealed_check(true)){
        moving_piece -> unmove(old_position, captured_piece_state);
        if (selected_move.k_castle || selected_move.q_castle){
            castling_rook -> unmove(old_rook_position, std::make_pair(false,castling_rook)); // Note: the pair is not needed in this case, so it can be fed default values.
        }
        return true;
     
    } else {
        // Delete the captured piece, if one was captured
        if (captured_piece_state.first){
            std::cout<<"Deleting captured piece: "<<piece_to_char(captured_piece_state.second->get_abbrev())<<std::endl;
            if (captured_piece_state.second->get_abbrev()==chess_vars::king){
                throw KingDeletionException();
            }
            (*occupied).erase(captured_piece_state.second->location());
            delete captured_piece_state.second; // Note: even if capture is not explicitly provided in move request, will delete if space was previously occupied

        }

        // Elegant way: generate moves for this piece 
        // and remove this new position from the allowed moves of my own pieces
        // Currently: delete all data contained in allowed moves and destinations;
        // EDIT: not that straightforward -> move can break lines of sight, thus removing multiple moves
        
        // Update en-passant state : check if last move was a pawn double jump
        if (moving_piece->get_abbrev()==chess_vars::pawn && abs(old_position.y()-selected_move.end.y())==2 && old_position.x()-selected_move.end.x()==0){
            piece::set_legal_en_passant(true, moving_piece->location()); // Update position of pawn to capture
        } else {
            piece::set_legal_en_passant(false, position(0,0));
        }

        return false;
    }
}


// Generate threeats for all of  current player's pieces
void chess::generate_threats()
{
    // IMPORTANT: we may only want to distinguish between the two when dealing with pawns and kings
    // All other pieces do not have moves which cannot capture.
    
    // First: reset the threats for this player
    piece::reset_threats(current_player);    

    for (int x{1}; x<=8; x++){
        for (int y{1}; y<=8; y++){
            if ( (*occupied).count(position(x,y))){
                //std::cout<<"Generating threats for "<<position(x,y)<<std::endl;
                piece *temp {(*occupied).at(position(x,y))};
                
                if (temp->get_owner()!=current_player){
                    continue;
                }
                (*temp).generate_threats();
            }
        }
    }


}

// Request the chess_board to print an update: only useful if board needs printing outside of a member function
void chess::print_board()
{
    chess_board.print_board();
}

// End of turn: generate moves and threats and check for endgame
void chess::update_game_status()
{
    // If game.get_request() == move:
    // If user request is something other than a move: no need to update game status: exit! 
    std::cout<<"Entered update...\n";
    system("clear");
    if (current_request!=chess_vars::move ){
        return;
    }

    //Currently only prints board
    chess_board.print_board();
    
    // If initialising a non-default board: need to switch to previous player to generate threats and check for end of game scenario.
    if (!initialisation_requested){
        initialisation_requested = true;
        std::cout<<"Update status: running on initialisation."<<std::endl;
        current_player = switch_player(current_player);
    }

    // First calculate threats + defences
    this -> generate_threats(); 
    // Switch player
    current_player = switch_player(current_player); 
    std::cout<<"Current player: "<<color_to_char(current_player)<<std::endl;   
    // Then calculate allowed moves (note: this is also done once during initialisation)
    this -> generate_moves();
    
    // If opponent checked: check for checkmate or stalemate
    chess_vars::check_status status { (*the_kings)[current_player]->is_checkmated() };
    switch (status)
    {
    case chess_vars::nominal:
        std::cout<<"Nominal..."<<std::endl;
        break;
    case chess_vars::check:
        /* Proceed to next move */
        std::cout<<"CHECK!! CAREFUL..."<<std::endl;
        break;
    case chess_vars::checkmate:
        // Current player has lost. Update game status.
        current_status = chess_vars::game_over;
        if (current_player==chess_vars::black){
            outcome = chess_vars::white_won;
        } else {
            outcome = chess_vars::black_won;
        }
        std::cout<<"CHECKMATE!!"<<std::endl;
        break;
    case chess_vars::stalemate:
        // Current player has suffered a stalemate. Update game status.
        current_status = chess_vars::game_over;
        outcome = chess_vars::draw_by_stalemate;
        std::cout<<"STALEMATE !!"<<std::endl;
        break;
    default: // Not accessible
        // TS: error handling
        break;
    }
    
}


// Request path from user for either save location or load location 
std::string get_path(std::string current_save_location, bool loading)
{
    std::string verb, pronoun,warning;
    if (loading){
        verb = "Load";
        pronoun = "from";
        warning = "Warning: loading a file will erase the current game, even if file is compromised.\n";
    } else{
        verb = "Save";
        pronoun = "to";
        warning = "";
    }
    bool valid_path{false};
    std::string current_path;
    std::stringstream user_msg;
    if (current_save_location.length()>0){
        user_msg << "Current "<<get_lower(verb)<<" location: " << current_save_location <<"\n";
    }
    std::cout<<warning;
    user_msg << verb << " game "<< pronoun<<" file [ (y)es / (n)o / (e)dit ]: "; 
    std::string error_msg { "Invalid option!"};
    std::vector<std::string> input_options { "yes", "no", "edit"};

    system("clear");  
    int lines_to_clear{2};  
    while (!valid_path){
        char answer { ask_user_word(user_msg.str(),error_msg,input_options) };

        switch (answer)
        {
        case 'y':
            if (parent_folder_exists(current_save_location)){
                valid_path = true;
            }
            return current_save_location;
        case 'n':
            return "";
        case 'e':
            //editing of path by user
            std::cout<<"Provide a valid path.";
            if (!loading) std::cout<<" If file does not exits, one will be created: ";
            //std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
            std::cin.clear();
            while(getline(std::cin, current_path)){
                break;
            };
            std::cout<<"Chosen path: "<<current_path<<std::endl;
            if (parent_folder_exists(current_path)){
                //valid_path = true;
                return current_path;
            }
            break;
        default: // Not accessible
            break;
        }
        lines_to_clear = 3;
        if (!valid_path) {
            system("clear");
            std::cout<<"Parent folder not found! Try again.\n";
        }
    }
    // If no other valid options: clumsy exit
    return "";
}

void chess::save_game()
{
    // Note: smart path finding implemented in later version
    // Note: would still need to test that there is indeed something to save
    std::string requested_location {get_path(save_location,false)};
    if (requested_location.length()==0){
        return;
    } else {
        save_location = requested_location;
    }

    std::ofstream file(save_location);
    // Save current status of game to file
    file << "---GAME_STATUS---" << std::endl;
    file << current_player<<std::endl;
    file << current_status <<std::endl;
    file << "---BEGIN_POSITIONS---" << std::endl;
    for (std::map<position, piece*>::iterator iter= (*occupied).begin(); iter!=(*occupied).end(); ++iter){
        piece* temp {iter->second};
        file << temp->location().x() <<" "<< temp->location().y() <<" "<< temp->get_owner() << " "<< temp->get_abbrev()<<std::endl;
    }
    file << "---MOVE_HISTORY---" << std::endl; //TS: not currently supported
    
    file.close();
}

// Read in custom file format
// PLEASE NOTE: I somehow managed to break this in the last few hours before this assignment was due.
// It would appear the error is related to the file skipping lines and reading lines twice. I've left troublshooting statements in to aid in deciphering the problem.
void chess::load_game()
{
    std::string requested_location {get_path(save_location,true)};
    if (requested_location.length()==0){
        return;
    } else {
        save_location = requested_location;
    }
    // Hard reset of game: required, even if loading fails
    piece::reset_occupied_spaces();

    std::fstream file(save_location);
    std::stringstream input_stream;
    std::string line;
    std::string header{"game_status"};
    std::string position_line{"begin_positions"};
    std::string moves_line{"move_history"};

    if (!file.is_open()){
        throw LoadFileException("File did not open.");
    }
    getline(file,line);
    to_lower(line);
    if (line.find(header,0)==std::string::npos){
        throw LoadFileException("File structure was not recognised: expected \""+header+"\"");
    }
    int player_, status_;
    file >> player_ >> status_;
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    chess_vars::player_color temp_color { static_cast<chess_vars::player_color>(player_)};
    chess_vars::game_status temp_status { static_cast<chess_vars::game_status>(status_)};     
    switch (temp_color)
    {
    case chess_vars::white:
    case chess_vars::black:
        break;
    default:
        throw InvalidEnum(); // If no valid color recognised
    }
    switch (temp_status)
    {
    case chess_vars::game_on:
    case chess_vars::game_over:
        break;        
    default:
        throw InvalidEnum(); // If no valid game state recognised
    }

    // Look for header line
    getline(file,line);
    to_lower(line);
    if (line.find(position_line,0)==std::string::npos){
        throw LoadFileException("Expected \""+position_line+"\", but instead found: "+line);
    }

    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    int x_, y_, owner_, abbrev_;
    position that_pos;
    chess_vars::player_color that_color;
    chess_vars::piece_type that_piece;
    
    int count_of_pieces{};
    std::map<chess_vars::player_color, king*> loaded_kings;
    
    loaded_positions.clear();
    while(line.find(moves_line,0)==std::string::npos){
        file >> x_ >> y_ >> owner_ >> abbrev_;
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        that_pos = position(x_,y_);
        std::cout<<"That Pos: "<<that_pos<<std::endl;
        if (!that_pos.is_valid()){
            throw InvalidPosition();
        } else if (loaded_positions.count(that_pos)>0){
            throw LoadFileException(); // Redundant check (see below), but verify if already a piece here
        }
        that_color = static_cast<chess_vars::player_color>(owner_);
        that_piece = static_cast<chess_vars::piece_type>(abbrev_);     
        switch (that_color)
        {
        case chess_vars::white:
        case chess_vars::black:
            break;
        default:
            throw InvalidEnum();
        }
        switch (that_piece)
        {
        case chess_vars::pawn:
        case chess_vars::rook:
        case chess_vars::knight:
        case chess_vars::bishop:
        case chess_vars::queen:
        case chess_vars::king:
            break;
        default:
            throw InvalidEnum();
        }
        if (loaded_positions.count(that_pos)>0){
            throw OvercrowdedPosition(that_pos.x(),that_pos.y());            
        }
        loaded_positions[that_pos] = piece_initialiser(that_color, that_piece, that_pos);
        if (that_piece==chess_vars::king){
            if (loaded_kings.count(that_color)!=0){
                throw TooManyKingsException(that_color);
            } else {
                loaded_kings[that_color] = dynamic_cast<king*>(loaded_positions[that_pos]);
            }
        }
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        count_of_pieces++;
    }
    // Check that there is one king for each player.
    if ( loaded_kings.count(chess_vars::white)==0 ){
        throw NotEnoughKingsException(chess_vars::white);
    } else if (loaded_kings.count(chess_vars::black)==0){
        throw NotEnoughKingsException(chess_vars::black);
    }

    // Alert user to loaded configuration
    std::cout<<"Current settings: "<<(current_player==chess_vars::white) <<" "<< (current_status==chess_vars::game_on)<<std::endl;
    std::cout<<"Number of pieces: "<<count_of_pieces<<std::endl;

    //Perform sanity test on loaded_positions before copying over data to 'occupied'.    
    //Need to check that 
    //  - no pawns left hanging unpromoted on eigth rank
    //  - the previous player has not left their position in a state of check
    //  - ...?
    
    // Check for hanging unpromoted pawns:
    chess_vars::player_color backrank_color { chess_vars::white};
    int that_rank {8};
    position check_position;
    for (int player_i{}; player_i<2; player_i++){
        for (int x{1}; x<=8; x++){
            check_position = position(x,that_rank);
            if (loaded_positions.count(check_position)){
                if (loaded_positions.at(check_position)->get_owner()==backrank_color && loaded_positions.at(check_position)->get_abbrev()==chess_vars::pawn){
                    std::stringstream msg {"You've loaded a pawn to the last rank on file "};
                    msg << check_position.x() << ". Choose what to promote it to: (Q)ueen, (R)ook, (B)ishop, k(N)ight, (E)xit: ";
                    std::string error_msg {"Not a recognised piece."};
                    std::vector<std::string> input_options {"q", "b", "r","n"};
                    char promotion { ask_user_word(msg.str(),error_msg, input_options)};
                    chess_vars::piece_type promotion_type;
                    switch (promotion)
                    {
                    case 'e':
                    case 'q':
                        promotion_type = chess_vars::queen;
                        break;
                    case 'b':
                        promotion_type = chess_vars::bishop;
                        break;
                    case 'r':
                        promotion_type = chess_vars::rook;
                        break;
                    case 'n':
                        promotion_type = chess_vars::knight;
                        break;
                    default:
                        break;
                    }
                    delete loaded_positions.at(check_position);
                    loaded_positions.erase(check_position);
                    piece* temp {promote_piece(current_player, check_position, promotion_type)};
                    loaded_positions[check_position] = temp;
                }
            }
        }
        that_rank = 1;
        that_color = chess_vars::black;
    }

    // Check if game status has been left in check.
    piece::reset_threats(temp_color);  
    piece::reset_threats(switch_player(temp_color));  
    
    // Reset and generate threats for both players (although probably sufficient just to reset/restart the enemy threats).
    for (std::map<position, piece*>::iterator iter= loaded_positions.begin(); iter!=loaded_positions.end(); ++iter){
        piece* temp {iter->second};
        (*temp).generate_threats();
    }

    // Check that the previous player's king was not left in a threatened square: if they are, this would not picked up by is_checkmated().
    if (piece::is_threatened(loaded_kings[switch_player(temp_color)])){
        throw LoadFileException("The previous player has left their king in a state of check. The game position is thus invalid.");
    }

    // All in order. Game status will be checked on initialisation. Perform transaction to game variables.
    current_player = temp_color;
    current_status = temp_status;
    occupied = &loaded_positions;
    the_kings = &loaded_kings;
    initialisation_requested = true;

    file.close();
}

chess_vars::request chess::get_request()
{
    return current_request;
}

chess_vars::setup chess::get_setup()
{
    return setup_type;
}

