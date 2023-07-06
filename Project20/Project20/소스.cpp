#include "lodepng.h"
#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <stack>
//#include <algorithm>
using namespace std;

void encodeBMP(std::vector<unsigned char>& bmp, const unsigned char* image, int w, int h) {
	//3 bytes per pixel used for both input and output.
	int inputChannels = 4;
	int outputChannels = 3;

	//bytes 0-13
	bmp.push_back('B'); bmp.push_back('M'); //0: bfType
	bmp.push_back(0); bmp.push_back(0); bmp.push_back(0); bmp.push_back(0); //2: bfSize; size not yet known for now, filled in later.
	bmp.push_back(0); bmp.push_back(0); //6: bfReserved1
	bmp.push_back(0); bmp.push_back(0); //8: bfReserved2
	bmp.push_back(54 % 256); bmp.push_back(54 / 256); bmp.push_back(0); bmp.push_back(0); //10: bfOffBits (54 header bytes)

	//bytes 14-53
	bmp.push_back(40); bmp.push_back(0); bmp.push_back(0); bmp.push_back(0);  //14: biSize
	bmp.push_back(w % 256); bmp.push_back(w / 256); bmp.push_back(0); bmp.push_back(0); //18: biWidth
	bmp.push_back(h % 256); bmp.push_back(h / 256); bmp.push_back(0); bmp.push_back(0); //22: biHeight
	bmp.push_back(1); bmp.push_back(0); //26: biPlanes
	bmp.push_back(outputChannels * 8); bmp.push_back(0); //28: biBitCount
	bmp.push_back(0); bmp.push_back(0); bmp.push_back(0); bmp.push_back(0);  //30: biCompression
	bmp.push_back(0); bmp.push_back(0); bmp.push_back(0); bmp.push_back(0);  //34: biSizeImage
	bmp.push_back(0); bmp.push_back(0); bmp.push_back(0); bmp.push_back(0);  //38: biXPelsPerMeter
	bmp.push_back(0); bmp.push_back(0); bmp.push_back(0); bmp.push_back(0);  //42: biYPelsPerMeter
	bmp.push_back(0); bmp.push_back(0); bmp.push_back(0); bmp.push_back(0);  //46: biClrUsed
	bmp.push_back(0); bmp.push_back(0); bmp.push_back(0); bmp.push_back(0);  //50: biClrImportant

	/*
	Convert the input RGBRGBRGB pixel buffer to the BMP pixel buffer format. There are 3 differences with the input buffer:
	-BMP stores the rows inversed, from bottom to top
	-BMP stores the color channels in BGR instead of RGB order
	-BMP requires each row to have a multiple of 4 bytes, so sometimes padding bytes are added between rows
	*/

	int imagerowbytes = outputChannels * w;
	imagerowbytes = imagerowbytes % 4 == 0 ? imagerowbytes : imagerowbytes + (4 - imagerowbytes % 4); //must be multiple of 4

	for (int y = h - 1; y >= 0; y--) { //the rows are stored inversed in bmp
		int c = 0;
		for (int x = 0; x < imagerowbytes; x++) {
			int index = inputChannels * (w * y + x / outputChannels);

			if (x < w * outputChannels)
			{
				int offset = c;
				//Convert RGB(A) into BGR(A)
				if (c == 0) offset = 2;
				else if (c == 2) offset = 0;


				// png의 a 값이 0이면
				if (image[index + 3] <= 0)
				{
					// 현재 읽어온 값이 r or b 인 경우
					if (c == 0 || c == 2)
						bmp.push_back(255); // 255 대입
					else // g 인 경우
						bmp.push_back(0); // 0 대입
				}
				// png의 a 값이 0이 아닌 경우
				else
					// png rgb 값 그대로 대입
					bmp.push_back(image[index + offset]);
			}
			else
				bmp.push_back(0);
			c++;
			if (c >= outputChannels) c = 0;
		}
	}

	// Fill in the size
	bmp[2] = bmp.size() % 256;
	bmp[3] = (bmp.size() / 256) % 256;
	bmp[4] = (bmp.size() / 65536) % 256;
	bmp[5] = bmp.size() / 16777216;
}
bool FindSameFile(std::string _cur_path,std::string _filename)
{
	bool flag = false;
	for (auto& itr : std::filesystem::recursive_directory_iterator(_cur_path))
	{
		if (std::filesystem::is_directory(itr.path()) == true)
		{
			continue;
		}
		std::string str = itr.path().filename().string();
		if (str.compare(_filename) == 0)
		{
			flag = true;
			break;
		}
	}

	return flag;
}
void CreateFolderBitmap(const std::string& _path)
{
	for (auto& itr : std::filesystem::recursive_directory_iterator(_path))
	{
		if (std::filesystem::is_directory(itr.path())==true)
		{
			continue;
		}
		std::string file_name = itr.path().filename().string();
		std::string full_name = _path + file_name;

		const std::wstring ext = itr.path().extension();
		if (ext != L".png")
			continue;

		std::vector<unsigned char> image; //the raw pixels
		unsigned width, height;
		unsigned error = lodepng::decode(image, width, height, full_name, LCT_RGBA, 8);
		int size = full_name.size() - 1;
		full_name[size - 2] = L'b';
		full_name[size - 1] = L'm';
		full_name[size] = L'p';
		int size2 = file_name.size() - 1;
		file_name[size2 - 2] = L'b';
		file_name[size2 - 1] = L'm';
		file_name[size2] = L'p';
	    if (error && FindSameFile(_path, file_name) == true)
		{
			continue;
		}
		if (error)
		{
			std::cout << "error " << error << ": " << lodepng_error_text(error) << std::endl;
			return;
		}

		std::vector<unsigned char> bmp;
		encodeBMP(bmp, &image[0], width, height);

		
		lodepng::save_file(bmp, full_name);
	}
}
std::stack<string> directory_path;
void CreateFoldersBitmap(const std::string& _root_path)
{
	bool file_read_flag = false;
	for (auto& itr : std::filesystem::recursive_directory_iterator(_root_path))
	{
		std::string outfilename_str = itr.path().string();
		outfilename_str += "\\";
		const std::wstring ext = itr.path().extension();
		if (std::filesystem::is_directory(outfilename_str))
		{
			CreateFolderBitmap(outfilename_str);
		}
	}
	CreateFolderBitmap(_root_path + "\\");
}
int main()
{
	const std::string path = "C:\\Users\\skagu\\OneDrive\\Desktop\\Yeram\\CreateNPKFile\\CreateNPKFile\\NPKTestFile\\png2bmpImages";
	CreateFoldersBitmap(path);
	return 0;
}