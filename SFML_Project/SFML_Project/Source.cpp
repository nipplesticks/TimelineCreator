#include <SFML/Graphics.hpp>
#include "Utility/Timer.h"
#include <fstream>
#include <string>
#include <vector>
#include <windows.h>
#include <sstream>


struct DataSample
{
	UINT64 TickStart = 0;
	UINT64 TickEnd = 0;
	UINT64 TickDelta = 0;
	double Period = 0.0;

	bool operator<(const DataSample & other) const
	{
		return TickStart < other.TickStart;
	}
};

struct DataFile
{
	std::string Name;
	std::vector<DataSample> Data;

	void Sort()
	{
		std::sort(Data.begin(), Data.end());
	}

	bool operator<(const DataFile & other) const
	{
		return Data.front() < other.Data.front();
	}
};

struct Line
{
	sf::Vertex line[2];
};

std::vector<DataFile> LoadData();

UINT FitData(std::vector<DataFile> & data);

void CalcReferenceAndDelta(std::vector<DataFile> & sortedData, UINT samples);

void CreateDiagram(const std::vector<DataFile> & data, UINT samples);

struct ScreenSize
{
	UINT x = 0, y = 0;
};


const std::string PATH = "../Data/";

ScreenSize gScreenSize;

int main()
{
	std::vector<DataFile> data = LoadData();
	std::sort(data.begin(), data.end());

	UINT samples = FitData(data);

	CalcReferenceAndDelta(data, samples);

	CreateDiagram(data, samples);

	return 0;
}

std::vector<DataFile> LoadData()
{
	std::vector<DataFile> allData;

	std::ifstream loadFiles;
	loadFiles.open(PATH + "Input.txt");


	{
		std::string res;
		std::getline(loadFiles, res);
		std::stringstream ss(res);

		ss >> gScreenSize.x;
		ss >> gScreenSize.y;
	}


	std::string fileName;
	while (std::getline(loadFiles, fileName))
	{
		DataFile df;
		df.Name = std::string(fileName.begin(), fileName.begin() + fileName.find_last_of('.'));
		std::ifstream f;
		f.open(PATH + fileName);
		std::string dataString;
		while (std::getline(f, dataString))
		{
			std::stringstream ss(dataString);
			DataSample dp;

			ss >> dp.TickStart;
			ss >> dp.TickEnd;
			ss >> dp.Period;

			df.Data.push_back(dp);
		}
		f.close();

		df.Sort();

		allData.push_back(df);
	}
	loadFiles.close();


	return allData;
}

UINT FitData(std::vector<DataFile>& data)
{
	UINT Min = UINT_MAX;

	for (UINT i = 0; i < data.size(); i++)
	{
		size_t size = data[i].Data.size();

		if (size < Min)
		{
			Min = (UINT)size;
		}
	}


	for (UINT i = 0; i < data.size(); i++)
	{
		size_t size = data[i].Data.size();

		if (size > Min)
		{
			data[i].Data.erase(data[i].Data.begin() + Min, data[i].Data.end());
		}
	}

	return Min;
}

void CalcReferenceAndDelta(std::vector<DataFile>& sortedData, UINT samples)
{
	UINT size = sortedData.size();

	for (UINT s = 0; s < samples; s++)
	{
		UINT64 reference = sortedData.front().Data[s].TickStart;
		for (UINT d = 0; d < size; d++)
		{
			sortedData[d].Data[s].TickStart -= reference;
			sortedData[d].Data[s].TickEnd -= reference;
			sortedData[d].Data[s].TickDelta = sortedData[d].Data[s].TickEnd - sortedData[d].Data[s].TickStart;
			
		}
	}
}

void CreateDiagram(const std::vector<DataFile>& data, UINT samples)
{
	std::vector<sf::Image> Images(samples);


	sf::RectangleShape background;
	background.setSize(sf::Vector2f(gScreenSize.x, gScreenSize.y));
	background.setFillColor(sf::Color::White);

	size_t numberOfBoxes = data.size();

	std::vector<sf::RectangleShape> boxes(numberOfBoxes);

	for (int i = 0; i < numberOfBoxes; i++)
	{
		sf::Color c(rand() % 256, rand() % 256, rand() % 256);
		boxes[i].setFillColor(c);
		boxes[i].setOutlineThickness(-2.0f);
		boxes[i].setOutlineColor(sf::Color::Black);
	}

	sf::RenderTexture rTex;
	rTex.create(gScreenSize.x, gScreenSize.y);

	float offset = 128;

	float boxHeight = ((float)gScreenSize.y - offset) / (float)numberOfBoxes;

	//for (UINT s = 0; s < 1; s++)
	for (UINT s = 0; s < samples; s++)
	{
		long double Max = data.back().Data[s].TickEnd;

		for (size_t box = 0; box < numberOfBoxes; box++)
		{
			if (s == 19 && box == 1)
				int lol = 2;


			sf::Vector2f pos;
			pos.y = boxHeight * box + offset * 0.5f;
			pos.x = ((long double)data[box].Data.at(s).TickStart / Max) * ((double)gScreenSize.x - offset);

			if (pos.x < 0.0f)
				pos.x = 0.0f;

			if (pos.x > gScreenSize.x)
				pos.x = 0.0f;

			sf::Vector2f size;
			size.y = boxHeight;
			size.x = ((long double)data[box].Data.at(s).TickDelta / Max) * ((double)gScreenSize.x - offset);

			if (size.x < 4.0f)
				size.x = 4.0f;

			boxes[box].setPosition(pos);
			boxes[box].setSize(size);
		}

		rTex.clear();
		rTex.draw(background);

		for (auto & b : boxes)
		{
			rTex.draw(b);
		}
		rTex.display();
		
		Images[s] = rTex.getTexture().copyToImage();
	}

	for (int i = 0; i < samples; i++)
	{
		Images[i].saveToFile(PATH + "diagram_" + std::to_string(i) + ".bmp");
	}

}
