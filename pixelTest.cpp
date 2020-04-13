#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <vector>
#include <unistd.h>

class PixelBackEnd
{
public:
	PixelBackEnd(int width, int height) :
	width(width),
	height(height),
	data(width*height),
	oldData(data),
	stride(width)
	{
	}
	
	void set(int x, int y, bool value)
	{
		data[x*stride + y] = value;
	}

	bool get(int x, int y) const
	{
		return oldData[x*stride + y];
	}	

	bool step()
	{
		for (int i = 0; i < width; ++i)
			for (int j = 0; j < height; ++j)
			{
				set(i, j, convOdd(i, j));
			}
		endStep();
	}

	void endStep()
	{
		oldData = std::move(data);
	}

	inline int mod(int x, int m) const {
	    int r = x%m;
	    return r<0 ? r+m : r;
	}

	bool convOdd(int x, int y) const
	{
		bool result = false;
		for (int i = -1; i <= 1; ++i)
			for (int j = -1; j <= 1; ++j)
			{
				//if (abs(i) != abs(j)) continue;
				if (!(i || j)) continue;
				int xp = (x + i);
				int yp = (y + j);
				if (xp >= width) continue; 
				if (yp >= height) continue; 
				if (xp <= 0) continue; 
				if (yp <= 0) continue; 
				result = result ^ get(xp, yp);
			}
		return result;
	}


private:
	const int height;
	const int width;
	std::vector<uint_fast8_t> data;
	std::vector<uint_fast8_t> oldData;
	const int stride;
};

class Example : public olc::PixelGameEngine
{
public:
	Example():
	scene(513, 513)
	{
		sAppName = "Example";
		scene.set(256, 256, true);
		scene.endStep();
	}

public:
	bool OnUserCreate() override
	{
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		for (int x = 0; x < ScreenWidth(); x++)
			for (int y = 0; y < ScreenHeight(); y++)
			{
				if (scene.get(x,y))
					Draw(x, y, olc::WHITE);
				else
					Draw(x, y, olc::BLACK);
			}
		scene.step();
		return true;
	}
private:
	PixelBackEnd scene;
};

int main()
{
	Example demo;
	if (demo.Construct(513, 513, 4, 4))
		demo.Start();
	return 0;
}