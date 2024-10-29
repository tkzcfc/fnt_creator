#pragma once

#include "Common.h"

struct PageRenderOpenglData
{
	std::vector<uint8_t> pixels;
	uint32_t texture_id;
	uint32_t width;
	uint32_t height;
};

class FntGen
{
public:

	FntGen();

	~FntGen();

	bool run(const GenerateConfig& config);

	void setEditorMode(bool value) { m_isEditorMode = value; }

	void setEditorShowPageIndex(int value) { m_editorShowPageIndex = value; }

	const PageRenderOpenglData& getPageRenderOpenglData() { return m_pageRenderOpenglData; }

	void clearPageRenderOpenglData()
	{
		m_pageRenderOpenglData.pixels.clear();
		m_pageRenderOpenglData.texture_id = 0;
		m_pageRenderOpenglData.width = 0;
		m_pageRenderOpenglData.height = 0;
	}

	bool supportGPU() { return m_context != nullptr; }

private:

	void matchFont(const GenerateConfig& config);

	bool draw(const GenerateConfig& config);

	void initPageData(const GenerateConfig& config, FntPage& page, int pageIndex);

	bool drawPage(const GenerateConfig& config, FntPage& page);

	bool saveBitmapToFile(const std::string& filename, SkBitmap& bitmap);

	void drawGlyphs(const GenerateConfig& config, FntPage& page, SkCanvas* canvas);

	bool saveFont(const GenerateConfig& config);


private:
	bool m_isEditorMode;
	int m_editorShowPageIndex;
	PageRenderOpenglData m_pageRenderOpenglData;
	std::string m_outFileName;
	int m_maxOffsetY;
	FntInfo m_fntInfo;

	sk_sp<GrDirectContext> m_context;
};
