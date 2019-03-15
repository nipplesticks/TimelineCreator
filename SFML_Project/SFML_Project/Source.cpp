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

float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

void CreateDiagram(const std::vector<DataFile> & data, UINT samples);

struct ScreenSize
{
	UINT x = 0, y = 0;
};


const std::string PATH = "../Data/";

ScreenSize gScreenSize;
sf::Font gFont;

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
	{
		std::string font;
		std::getline(loadFiles, font);
		gFont.loadFromFile(PATH + font);
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
	sf::RectangleShape background;
	background.setSize(sf::Vector2f(gScreenSize.x, gScreenSize.y));
	background.setFillColor(sf::Color::White);
	
	std::vector<sf::Image> Images(samples);

	size_t numberOfBoxes = data.size();
	std::vector<sf::RectangleShape> boxes(numberOfBoxes);
	std::vector<sf::RectangleShape> titles(numberOfBoxes);

	for (int i = 0; i < numberOfBoxes; i++)
	{
		sf::Color c(rand() % 256, rand() % 256, rand() % 256);
		boxes[i].setFillColor(c);
		boxes[i].setOutlineThickness(-2.0f);
		boxes[i].setOutlineColor(sf::Color::Black);
		titles[i] = boxes[i];
	}

	sf::RenderTexture rTex;
	rTex.create(gScreenSize.x, gScreenSize.y);

	float offsetY = 256;
	float offsetX = 128;

	float boxHeight = ((float)gScreenSize.y - offsetY) / (float)numberOfBoxes;

	std::vector<Line> lines(10);

	for (auto & l : lines)
	{
		l.line[0].color = sf::Color::Black;
		l.line[1].color = sf::Color::Black;	
	}

	for (UINT s = 0; s < 1; s++)
	//for (UINT s = 0; s < samples; s++)
	{
		long double Max = data.back().Data[s].TickEnd;

		rTex.clear();
		rTex.draw(background);

		
		float sPos = ((long double)data.front().Data.at(s).TickStart / Max) * ((double)gScreenSize.x - offsetX) + offsetX * 0.5f;
		float ePos = ((long double)data.back().Data.at(s).TickStart / Max) * ((double)gScreenSize.x - offsetX) + offsetX * 0.5f;
		float eSize = ((long double)data.back().Data.at(s).TickDelta / Max) * ((double)gScreenSize.x - offsetX);

		sf::Vector2f startLinePos1(sPos, offsetY * 0.9f);
		sf::Vector2f startLinePos2(sPos, gScreenSize.y);
		sf::Vector2f endLinePos1(ePos + eSize, offsetY * 0.9f);
		sf::Vector2f endLinePos2(ePos + eSize, gScreenSize.y);

		sf::Text text;
		text.setFont(gFont);
		text.setCharacterSize(16);
		text.setString(std::to_string(data.front().Data.at(s).TickStart));
		text.setFillColor(sf::Color::Black);
		auto bb = text.getLocalBounds();

		sf::Vector2f origin((float)bb.width * 0.5f, (float)bb.height * 0.5f);
		text.setOrigin(origin);

		lines.front().line[0].position = startLinePos1;
		lines.front().line[1].position = startLinePos2;
		text.setPosition(lines.front().line[0].position - sf::Vector2f(0, text.getCharacterSize()));

		rTex.draw(lines.front().line, 2, sf::Lines);
		rTex.draw(text);

		text.setString(std::to_string(data.back().Data.at(s).TickEnd));
		bb = text.getLocalBounds();
		origin = sf::Vector2f((float)bb.width * 0.5f, (float)bb.height * 0.5f);
		text.setOrigin(origin);

		lines.back().line[0].position = endLinePos1;
		lines.back().line[1].position = endLinePos2;

		text.setPosition(lines.back().line[0].position - sf::Vector2f(0, text.getCharacterSize()));
		rTex.draw(lines.back().line, 2, sf::Lines);
		rTex.draw(text);

		for (int i = 1; i < lines.size() - 1; i++)
		{
			float lerpVal = (float)i / (lines.size() - 1.0f);

			sf::Vector2f pos1(lerp(startLinePos1.x, endLinePos1.x, lerpVal), offsetY * 0.9);
			sf::Vector2f pos2(pos1.x, gScreenSize.y);
			lines[i].line[0].position = pos1;
			lines[i].line[1].position = pos2;
			//text.setString(std::to_string(data.back().Data.at(s).TickEnd));
			text.setString(std::to_string((int)(lerp((long double)data.front().Data.at(s).TickStart, (long double)data.back().Data.at(s).TickEnd, lerpVal) + 0.5f)));
			bb = text.getLocalBounds();
			origin = sf::Vector2f((float)bb.width * 0.5f, (float)bb.height * 0.5f);
			text.setOrigin(origin);
			text.setPosition(pos1 - sf::Vector2f(0, text.getCharacterSize()));

			rTex.draw(lines[i].line, 2, sf::Lines);
			rTex.draw(text);
		}


		for (size_t box = 0; box < numberOfBoxes; box++)
		{
			sf::Vector2f pos;
			pos.y = boxHeight * box + offsetY;
			pos.x = ((long double)data[box].Data.at(s).TickStart / Max) * ((double)gScreenSize.x - offsetX) + offsetX * 0.5f;

			sf::Vector2f size;
			size.y = boxHeight;
			size.x = ((long double)data[box].Data.at(s).TickDelta / Max) * ((double)gScreenSize.x - offsetX);


			if (pos.x < 0.0f)
				pos.x = 0.0f;

			if (pos.x > gScreenSize.x)
				pos.x = 0.0f;

			if (size.x < 4.0f)
				size.x = 4.0f;

			boxes[box].setPosition(pos);
			boxes[box].setSize(size);
			rTex.draw(boxes[box]);
		}

		rTex.display();
		
		Images[s] = rTex.getTexture().copyToImage();
	}

	for (int i = 0; i < samples; i++)
	{
		Images[i].saveToFile(PATH + "diagram_" + std::to_string(i) + ".bmp");
	}

}
