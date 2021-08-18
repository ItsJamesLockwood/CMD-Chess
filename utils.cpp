// Utilities file, part of the C++ Chess Project.
// Contains:
// - enums: codify variable names for chess states 
// - useful functions: user input, string handling,...
// - chess exception classes

#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <locale>
#include <map>
#include <deque>
#include <filesystem>
#include <sys/stat.h>
#include <exception>

#pragma once

#define YES 1
#define NO 0
//#define DEBUGMODE
//#define VERBOSE

#define USEICONS NO

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%% Utility functions %%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


// Check if element is in vector
template <typename T>
bool is_in(std::vector<T> &vect, T element)
{
    return (std::find(vect.begin(), vect.end(), element) != vect.end());
}

// Check if a char is in a string
bool char_in_str(std::string str, char c)
{
	return str.find(c)!=std::string::npos;
}

// Shortcut to clear the past n lines of the terminal
void clear_line(int n_times)
{
	for (int i{}; i<n_times; i++){
		std::cout << "\x1b[A"<<"\33[2K";		
	}
}

// Get the index of an element in a vector
template <typename T>
int get_index(std::vector<T> &vect, T element)
{
	typename std::vector<T>::iterator v_it { std::find(vect.begin(), vect.end(), element)};
	typename std::vector<T>::iterator v_begin {vect.begin()};
	if (v_it!=vect.end()){
		long int index { std::distance(v_begin,v_it)};
		return index;
	} else {
		return -1;
	}

}

// String operations
std::string& to_lower(std::string &input)
{
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);
    return input;
}

std::string get_lower(std::string input)
{
	std::string output {input};
    std::transform(output.begin(), output.end(), output.begin(), ::tolower);
	return output;
}
bool is_in_char(std::vector<std::string> &vect, char first_char)
{
	std::vector<std::string>::iterator s_begin {vect.begin()};
	std::vector<std::string>::iterator s_end {vect.end()};
	std::vector<std::string>::iterator s_it {};

	for (s_it=s_begin; s_it < s_end; ++s_it){
		if ((*s_it)[0] == first_char){
			return true;
		}
	}
	return false;
}


// Note: 
// - if only requesting one word: probably best to just use cin >> word;
// - after incorrect inputs: 2 options
//		- clear entire screen and print board + error message again
//		- go up a line and clear that line somehow (?)
// For now: will be satisfied with a multi-line option
char ask_user_word(std::string msg, std::string error_msg, std::vector<std::string> input_options, std::vector<std::string> quit_options={})
{
    std::string word;
	std::cout<< msg <<" ";
    while (true){
		getline(std::cin, word);
		//std::cin >> word;
		to_lower(word);
		if (is_in(input_options, word)){
			return word[0];
			break;
		} else if (word.size()==1) {
			if (is_in_char(input_options, word[0])){
				return word[0];
				break;
			}
		} else if (is_in(quit_options, word)) {
			return word[0];
			break;
		}
		clear_line(2);
		std::cout<<error_msg<<" "<<msg<<" ";

    }
} 


//TS: probably deprecated at this point, as now understand how getline works
// DEPRECATED: Get a line from the user 
template <typename T>
std::vector<T> ask_user_line(std::string error_msg)
{
    std::string line;
	std::stringstream input_stream;
	T element;
	std::vector<T> word_vector;

	getline(std::cin, line);
	input_stream.clear();
	input_stream.str(line);
	while (input_stream >> element){
		word_vector.push_back(element);
	}
    return word_vector;
} 


// Load a deque with items from a stringstream
template <typename T>
std::deque<T> user_load_queue(std::deque<T> &queue, std::string optional_msg=""){
	T element;
	std::string line;
	std::stringstream queue_stream;

	std::cout<<optional_msg;
	getline(std::cin, line);
	queue_stream.clear();
	queue_stream.str(line);
	while ( queue_stream >> element){
		queue.push_back(element);
		std::cout << "Element: "<<element<<"; Size: "<<element.size()<<std::endl;
	}
	return queue;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%% Chess variables %%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


namespace chess_vars {
    enum game_status{
        game_on = 0,
        game_over
    };
    enum game_outcome{
        white_won = 0,
        black_won = 1,
        draw_by_stalemate = 2,
        draw_by_offer = 3,
		ongoing
    };
    enum game_option{
        p_v_p = 1,
        p_v_computer = 2,
        c_v_computer,
        load_game,
        change_settings,
        quit_program
    };
	enum check_status{
		nominal = 0,
		check,
		checkmate,
		stalemate
	};
    enum player_color{
        black = 0,
        white = 1
    };
    enum request{
        move = 0,
        undo = 1,
        save = 2,
        offer_draw,
		remove_draw,
        resign,
        quit_game,
        menu,
		invalid_request
    };
    enum setup{
        default_board = 0,
        loaded_board
    };
    enum piece_type{
        pawn = 0,
        rook,
        knight,
        bishop,
        queen,
        king,
		nancy_rothwell // generic name for an undefined piece: "transcends other pieces" -> used as an exception clause
		// The name for the last enum is a bit random, but no appropriate could be found by the author when this was implemented. It has since remained and become a feature.
    };
	enum castle{
		no_castle = 0,
		q_castle,
		k_castle,
		both_castle
	};
};

// Convert from a char to an enum
chess_vars::piece_type char_to_piece(char id)
{
	switch(id){
		case ' ':
		case 'p':
		case 'P':
			return chess_vars::pawn;
		case 'r':
		case 'R':
			return chess_vars::rook;
		case 'n':
		case 'N':
			return chess_vars::knight;
		case 'B':
			return chess_vars::bishop;
		case 'q':
		case 'Q':
			return chess_vars::queen;
		case 'k':
		case 'K':
			return chess_vars::king;
		default:
			// Error handling
			std::cout<<"ERROR: you entered a non-valid piece id: "<<id<<std::endl;
			return chess_vars::nancy_rothwell;
			break;
	}
}

// Reverse operation of above function
char piece_to_char(chess_vars::piece_type type)
{
	switch (type)
	{
	case chess_vars::pawn:
		return 'P';
	case chess_vars::rook:
		return 'R';
	case chess_vars::knight:
		return 'N';
	case chess_vars::bishop:
		return 'B';
	case chess_vars::queen:
		return 'Q';
	case chess_vars::king:
		return 'K';
	case chess_vars::nancy_rothwell:
		return '?';	
	default:
		std::cout<<"ERROR: you entered a non-valid piece id.";
		return ' ';
		break;
	}
}

// Convert piece to Unicode icon
const char* piece_to_icon(chess_vars::piece_type type, chess_vars::player_color color)
{
	if (color==chess_vars::white){
		switch (type)
		{
		case chess_vars::pawn:
			return u8"\u2659";
		case chess_vars::rook:
			return u8"\u2656";
		case chess_vars::knight:
			return u8"\u2658";
		case chess_vars::bishop:
			return u8"\u2657";
		case chess_vars::queen:
			return u8"\u2655";
		case chess_vars::king:
			return u8"\u2654";
		case chess_vars::nancy_rothwell:
			return u8"\u25EF"; // Circle	
		default:
			std::cout<<"ERROR: you entered a non-valid piece id.";
			return u8"\u25EF"; // Circle
			break;
		}
	} else {
		switch (type)
		{
		case chess_vars::pawn:
			return u8"\u265F";
		case chess_vars::rook:
			return u8"\u265C";
		case chess_vars::knight:
			return u8"\u265E";
		case chess_vars::bishop:
			return u8"\u265D";
		case chess_vars::queen:
			return u8"\u265B";
		case chess_vars::king:
			return u8"\u265A";
		case chess_vars::nancy_rothwell:
			return u8"\u25CF"; // Circle	
		default:
			std::cout<<"ERROR: you entered a non-valid piece id.";
			return u8"\u25CF"; // Circle
			break;
		}
	}	
}

template <typename T>
chess_vars::player_color string_to_color(T input)
{
	if (input.length()>1){
		std::string color {get_lower(input)}; 
		if (color=="white"){
			return chess_vars::white;
		} else if (color=="black"){
			return chess_vars::black;
		} else {
			std::cerr<<"WARNING: unrecognised string: assuming you meant black!"<<std::endl;
			return chess_vars::black;
		}
	} else if (input.length()==1){
		char color { tolower(input[0])};
		switch (color)
		{
		case 'w':
			return chess_vars::white;		
		case 'b':
			return chess_vars::black;
		default:
			std::cerr <<"WARNING: unrecognised color character. Assumming black..."<<std::endl;
			return chess_vars::black;
		}
	} else {
		std::cerr<<"You gave no an input of length zero!"<<std::endl;
		exit(EXIT_FAILURE);
	}
}
char color_to_char(chess_vars::player_color color){
	switch (color)
	{
	case chess_vars::white:
		return 'W';
	case chess_vars::black:
		return 'B';
	default: // Should not be accessible
		return '?';
	}
}

std::string format_icon(char piece,char color)
{
    std::stringstream icon;
	if (color=='B' && piece!=' '){
		icon << "\033[1;30m" <<piece;// <<"\033[0m";
        return icon.str();
    } else if (color=='W' && piece!=' '){
        icon << "\033[1;35m" << piece; // << "\033[0m";
        return icon.str();
    } else {
        return " ";
    }
}


std::string format_icon(chess_vars::piece_type piece, chess_vars::player_color color)
{
	std::stringstream icon;
	std::string extra_spaces{};
#if USEICONS
	const char* piece_icon { piece_to_icon(piece, color) };
	extra_spaces = " ";
#else
	char piece_icon { piece_to_char(piece) };
	extra_spaces = "";
#endif
	if (color == chess_vars::black && piece!=chess_vars::nancy_rothwell){
		icon << "\033[1;30m" <<piece_icon << extra_spaces;// <<"\033[0m";
        return icon.str();
	} else if (color == chess_vars::white && piece!=chess_vars::nancy_rothwell){
		icon << "\033[1;35m" << piece_icon << extra_spaces; // << "\033[0m";
        return icon.str();
	} else {
		return " ";
	}
}


// Easily switch to the other player color
chess_vars::player_color switch_player(chess_vars::player_color current_player)
{
	chess_vars::player_color other_color;
	if (current_player == chess_vars::white){
		other_color = chess_vars::black;
	} else {
		other_color = chess_vars::white;
	}
	return other_color;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%% Loading functionalities %%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Check if a given path exists
// WARNING: not a stable solution: doesn't accout for all possibilities
bool path_exists(const std::string &s)
{
  struct stat buffer;
  return (stat (s.c_str(), &buffer) == 0);
}

// Check if a given folder exists
// WARNING: not a stable solution: doesn't accout for all possibilities
bool parent_folder_exists(std::string path)
{
	size_t end { path.length()};
	while (end>0){
		end--;
		if (path[end]=='/'){
			break;
		}
	}
	if (path_exists(path.substr(0,end)) || end==0){
		return true;
	} else {
		return false;	
	}
}


// Chess exceptions
// Parent exception: generic chess-related error
class ChessException : public std::exception 
{
	protected:
		std::string message{"Chess error message."};
	public:
		ChessException()= default;
		ChessException(std::string msg_): message{msg_}{}
		virtual ~ChessException() throw() {};
		virtual const char * what() const throw() 
		{
			//return "Chess Error: no further info.";
			return message.c_str();
		}
};

// Trying to read from an empty queue
class EmptyQueueException : public ChessException
{
	public:
		EmptyQueueException(){message="An issue lead to the moves deque being empty after user input!";};
		EmptyQueueException(std::string msg_)
		{
			message=msg_;
		}
};


// Generic loading-related exception
class LoadFileException : public ChessException 
{
	public:
		LoadFileException(){message="Problem while loading file...";};
		LoadFileException(std::string msg_)
		{
			message=msg_;
		}

};

// Did not recognise a piece?
class InvalidPiece : public ChessException 
{
	public:
		InvalidPiece(){message="Invalid piece...";};
		InvalidPiece(std::string msg_)
		{
			message=msg_;
		}
};

// Tried to deleted the king?
class KingDeletionException : public ChessException 
{
	public:
		KingDeletionException(){message="Attempted to delete a king. Halting any further moves...";};
		KingDeletionException(std::string msg_)
		{
			message=msg_;
		}
};

// Too many kings in loadfile
class TooManyKingsException: public LoadFileException
{
	public:
		TooManyKingsException(chess_vars::player_color col)
		{
			if (col==chess_vars::white){
				message = "File contains too many kings for player 'white'.";
			} else {
				message = "File contains too many kings for player 'black'.";
			}

		}
};

// Too few kings in loadfile
class NotEnoughKingsException: public LoadFileException
{
	public:
		NotEnoughKingsException(chess_vars::player_color col)
		{
			if (col==chess_vars::white){
				message = "File contains no king for player 'white'.";
			} else {
				message = "File contains no king for player 'black'.";
			}
		}
};

// Too many pieces on a square?
class OvercrowdedPosition: public LoadFileException
{
	public:
		OvercrowdedPosition(int x_, int y_)
		{
			std::stringstream msg {"File contains too many pieces for position ("};
			msg << x_ <<  ", " << y_ <<").";
			message = msg.str();
		}
};

// Invalid enum?
class InvalidEnum : public LoadFileException
{
	public:
		InvalidEnum(){message="Invalid enum...";};
		InvalidEnum(std::string msg_){message=msg_;}
};

class InvalidPosition: public LoadFileException
{
	public:
		InvalidPosition(){message="Invalid position...";};
		InvalidPosition(std::string msg_){message=msg_;}
};


// Messages
void print_welcome()
{
	
	std::cout<<std::string(50,'%')<<std::endl<<
		"_________  .__                              "<<std::endl<<
		"\\_   ___ \\ |  |__    ____    ______  ______ "<<std::endl<<
		"/    \\  \\/ |  |  \\ _/ __ \\  /  ___/ /  ___/ "<<std::endl<<
		"\\     \\____|   Y  \\  ___/  \\___ \\  \\___ \\  "<<std::endl<<
		"\\______  /|___|  / \\___  >/____  >/____  > "<<std::endl<<
		"	\\/      \\/      \\/      \\/      \\/  "<<std::endl<<
		std::string(50,'%')<<std::endl;

}

void exit_message()
{	
	// Goodbye!
	std::cout<<std::string(50,'%')<<std::endl<<
		"  ________                     .______.   "                 
		"/  _____/   ____    ____    __| _/\\_ |__  ___.__.  ____  " <<std::endl<<
		"/   \\  ___  /  _ \\  /  _ \\  / __ |  | __ \\<   |  |_/ __ \\ " <<std::endl<<
		"\\    \\_\\  \\(  <_> )(  <_> )/ /_/ |  | \\_\\ \\___  |\\  ___/  "<<std::endl<<
		"\\______  / \\____/  \\____/ \\____ |  |___  // ____| \\___  > "<<std::endl<<
		"		\\/                      \\/      \\/ \\/          \\/  "<<std::endl<<
		std::string(50,'%')<<std::endl;                                                    
}															
                                                           