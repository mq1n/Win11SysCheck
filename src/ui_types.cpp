#include "../include/pch.hpp"
#include "../include/ui_types.hpp"

namespace Win11SysCheck
{
	namespace gui
	{
		std::ostream& operator <<(std::ostream& stream, const point& pt)
		{
			stream << static_cast<std::string>(pt);
			return stream;
		}

		point::point() :
			x(0), y(0)
		{
		}
		point::point(int x_, int y_) :
			x(x_), y(y_)
		{
		}
		void point::operator ()(int x_, int y_)
		{
			this->x = x_;
			this->y = y_;
		}
		point::operator const std::string() const
		{
			std::stringstream ss;
			ss << "(" << this->x << ", " << this->y << ")";
			return ss.str();
		}

		size::size() :
			width(0), height(0)
		{
		}
		size::size(int width, int height) :
			width(width), height(height)
		{
		}

		rectangle::rectangle() :
			point(0, 0), size(0, 0)
		{
		}
		rectangle::rectangle(int x, int y, int width, int height) :
			point(x, y), size(width, height)
		{
		}
	}
};
