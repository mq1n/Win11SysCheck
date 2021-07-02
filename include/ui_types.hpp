#pragma once
#include <Windows.h>

namespace Win11SysCheck
{
	namespace gui
	{
		class point
		{
			friend std::ostream& operator <<(std::ostream& stream, point const& pt);

		public:
			point();
			point(int x, int y);

			void operator ()(int x, int y);
			operator const std::string() const;

			int x;
			int y;
		};

		struct size
		{
			size();
			size(int width, int height);

			int width;
			int height;
		};

		struct rectangle : public point, public size
		{
			rectangle();
			rectangle(int x, int y, int width, int height);
		};
	};
};
