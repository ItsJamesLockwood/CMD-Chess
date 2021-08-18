#include <iostream>
#pragma once

class position
{
    private:
        // The files a-h are given by x_pos=1-8. The ranks 1-8 are given by y_pos=1-8.
        int x_pos{}, y_pos{};
    public:
        position() = default;
        position(int xx, int yy): 
            x_pos{xx}, y_pos{yy} {}
        /*position(position &new_position)
        {
            x_pos = new_position.x_pos;
            y_pos = new_position.y_pos;
        }*/
        int x() const 
        {
            return x_pos;
        }
        int y() const 
        {
            return y_pos;
        }
        // Check that the position is a valid board coordinate
        bool is_valid() const 
        {
            if (x_pos>8 || x_pos<1){
                return false;
            } else if (y_pos>8 || y_pos<1){
                return false;
            } else {
                return true;
            }
        }
        void set(int xx, int yy)
        { 
            x_pos =xx;
            y_pos =yy;
        }
        
        position operator=(position new_pos)
        {
            if (&new_pos == this) return *this;

            x_pos = new_pos.x_pos;
            y_pos = new_pos.y_pos;
            return *this;
        }
        bool operator==(const position &new_pos) const
        {
            return (x_pos == new_pos.x_pos && y_pos == new_pos.y_pos);
        }
        position operator+(const position &increment) const  
        {
            position new_position;
            new_position.set(increment.x_pos + x_pos, increment.y_pos + y_pos);
            return new_position;
        }
        position operator*(int n) const
        {
            position new_position;
            new_position.set(x_pos * n, y_pos*n);
            return new_position;
        }
};  

std::ostream & operator<<(std::ostream &os, position pos)
{
    os << "x: " << pos.x() << "; y: " << pos.y();
    return os;            
}
// Require overload of operator< when using position as a key in a map. 
bool operator<(const position &p1, const position &p2)
{
    return p1.x() + 8*p1.y() < p2.x() + 8*p2.y();
}

