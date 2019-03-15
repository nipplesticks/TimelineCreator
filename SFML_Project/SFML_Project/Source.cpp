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
sf::Color gBackgroundColor;
sf::Texture gTex;

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
	{
		std::string texture;
		std::getline(loadFiles, texture);
		gTex.loadFromFile(PATH + "Assets/" + texture);
	}
	{
		std::string color;
		std::getline(loadFiles, color);
		std::stringstream ss(color);
		int r, g, b;
		ss >> r;
		ss >> g;
		ss >> b;

		gBackgroundColor.r = r;
		gBackgroundColor.g = g;
		gBackgroundColor.b = b;
		gBackgroundColor.a = 255;
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

#include <iostream>
void CreateDiagram(const std::vector<DataFile>& data, UINT samples)
{
	sf::RectangleShape background;
	background.setSize(sf::Vector2f(gScreenSize.x, gScreenSize.y));
	background.setFillColor(gBackgroundColor);
	
	std::vector<sf::Image> Images(samples);
	std::vector<bool> broken(samples);

	size_t numberOfBoxes = data.size();
	std::vector<sf::RectangleShape> boxes(numberOfBoxes);
	std::vector<sf::RectangleShape> titles(numberOfBoxes);
	std::vector<sf::Text> titlesTexts(numberOfBoxes);
	std::vector<Line> lines(20);

	for (int i = 0; i < numberOfBoxes; i++)
	{
		sf::Color c(rand() % 256, rand() % 256, rand() % 256);
		boxes[i].setFillColor(c);
		boxes[i].setOutlineThickness(-2.0f);
		boxes[i].setOutlineColor(sf::Color::Black);
		boxes[i].setTexture(&gTex);
		titles[i] = boxes[i];
	}

	sf::RenderTexture rTex;
	rTex.create(gScreenSize.x, gScreenSize.y);

	float offsetY = 256;
	float offsetX = 128;

	float boxHeight = ((float)gScreenSize.y - offsetY) / (float)numberOfBoxes;


	for (auto & l : lines)
	{
		l.line[0].color = sf::Color::Black;
		l.line[1].color = sf::Color::Black;	
	}

	std::cout << "Creating Chars...\n";
	for (UINT s = 0; s < samples; s++)
	{
		float precent = (((float)s + 1.0f) / float(samples)) * 100.0f;
		std::cout << "\r" << precent << " %";

		rTex.clear();
		rTex.draw(background);

#pragma region titles
		sf::Vector2f tStartPos(0, offsetY * 0.3f);
		float textOffset = 0;
		
		for (int ti = 0; ti < titles.size(); ti++)
		{
			sf::Text tText;
			tText.setFont(gFont);
			tText.setCharacterSize(16);
			tText.setString(data[ti].Name);
			tText.setFillColor(sf::Color::Black);
			auto bb = tText.getLocalBounds();
			sf::Vector2f origin(0.0f, (float)bb.height * 0.5f);
			tText.setOrigin(origin);

			titles[ti].setPosition(sf::Vector2f(textOffset, 0) + tStartPos);
			titles[ti].setSize(sf::Vector2f(32, 32));

			tText.setPosition(sf::Vector2f(textOffset, 0.0f) + sf::Vector2f(titles[ti].getSize().x + 8, titles[ti].getSize().y * 0.5f) + tStartPos);

			textOffset = tText.getPosition().x + bb.width + 16.0 - tStartPos.x;
			titlesTexts[ti] = tText;
		}
		float titleSize = gScreenSize.x - textOffset;

		tStartPos = sf::Vector2f(titleSize * 0.5f, offsetY * 0.3f);
		textOffset = 0;
		for (int ti = 0; ti < titles.size(); ti++)
		{			
			auto bb = titlesTexts[ti].getLocalBounds();
			titles[ti].setPosition(sf::Vector2f(textOffset, 0) + tStartPos);

			titlesTexts[ti].setPosition(sf::Vector2f(textOffset, 0.0f) + sf::Vector2f(titles[ti].getSize().x + 8, titles[ti].getSize().y * 0.5f) + tStartPos);

			textOffset = titlesTexts[ti].getPosition().x + bb.width + 16.0 - tStartPos.x;
		
			rTex.draw(titles[ti]);
			rTex.draw(titlesTexts[ti]);
		}



#pragma endregion
		long double Max = data.back().Data[s].TickEnd;
#pragma region lines
		float sPos = ((long double)data.front().Data.at(s).TickStart / Max) * ((double)gScreenSize.x - offsetX) + offsetX * 0.5f;
		float ePos = ((long double)data.back().Data.at(s).TickStart / Max) * ((double)gScreenSize.x - offsetX) + offsetX * 0.5f;
		float eSize = ((long double)data.back().Data.at(s).TickDelta / Max) * ((double)gScreenSize.x - offsetX);

		sf::Vector2f startLinePos1(sPos, offsetY * 0.8f);
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

			float useOffset = offsetY * 0.9f;

			if (i % 2 == 0)
				useOffset = offsetY * 0.8f;



			sf::Vector2f pos1(lerp(startLinePos1.x, endLinePos1.x, lerpVal), useOffset);
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

#pragma endregion

#pragma region timeline
		float blankSpaceY = 32.0f;
		for (size_t box = 0; box < numberOfBoxes; box++)
		{
			sf::RectangleShape rs;
			sf::Color col = gBackgroundColor;
			col.a = 200;
			rs.setFillColor(col);
			Line l;
			sf::Text ms;
			ms.setFont(gFont);
			ms.setString(std::to_string(data[box].Data[s].Period) + " ms");
			ms.setCharacterSize(16);
			ms.setFillColor(sf::Color::Black);
			auto bb = ms.getLocalBounds();
			sf::Vector2f origin((float)bb.width * 0.5f, (float)bb.height * 0.5f);
			ms.setOrigin(origin);

			sf::Vector2f pos;
			pos.y = boxHeight * box + offsetY;
			pos.x = ((long double)data[box].Data.at(s).TickStart / Max) * ((double)gScreenSize.x - offsetX) + offsetX * 0.5f;

			sf::Vector2f size;
			size.y = boxHeight - blankSpaceY;
			size.x = ((long double)data[box].Data.at(s).TickDelta / Max) * ((double)gScreenSize.x - offsetX);

			ms.setPosition(pos.x + size.x * 0.5f, pos.y - 20.0f);

			if (pos.x < 0.0f)
				broken[s] = true;

			if (pos.x > gScreenSize.x)
				broken[s] = true;

			if (size.x < 4.0f)
				size.x = 4.0f;

			l.line[0].color = sf::Color::Black;
			l.line[1].color = sf::Color::Black;
			l.line[0].position = pos - sf::Vector2f(0, 3.0f);
			l.line[1].position = l.line[0].position + sf::Vector2f(size.x, 0.0f);

			bb = ms.getGlobalBounds();

			if (size.x < bb.width)
			{
				rs.setPosition(sf::Vector2f(bb.left, pos.y) - sf::Vector2f(0, 32));
				rs.setSize(sf::Vector2f(bb.width, size.y) + sf::Vector2f(0, 32));
			}
			else
			{
				rs.setPosition(pos - sf::Vector2f(0, 32));
				rs.setSize(size + sf::Vector2f(0, 32));
			}

			boxes[box].setPosition(pos);
			boxes[box].setSize(size);
			
			rTex.draw(rs);
			rTex.draw(boxes[box]);
			rTex.draw(l.line, 2, sf::Lines);
			rTex.draw(ms);
		}
#pragma endregion

		rTex.display();
		
		Images[s] = rTex.getTexture().copyToImage();
	}


	std::cout << "Writing to disk...\n";
	for (int i = 0; i < samples; i++)
	{
		float precent = (((float)i + 1.0f) / float(samples)) * 100.0f;
		std::cout << "\r" << precent << " %";


		if (broken[i])
			Images[i].saveToFile(PATH + "Chart_" + std::to_string(i) + "_broken.bmp");
		else
			Images[i].saveToFile(PATH + "Chart_" + std::to_string(i) + ".bmp");
	}

}
