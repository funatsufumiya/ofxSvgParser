//
//  ofxSvgLoader.h
//
//  Created by Nick Hardeman on 8/31/24.
//

#pragma once
#include "ofxSvgGroup.h"
#include "ofXml.h"
#include "ofxSvgCss.h"

namespace ofx::svg {
class Parser : public Group {
public:
	
	virtual SvgType getType() override {return TYPE_DOCUMENT;}
	
	bool load( of::filesystem::path aPathToSvg );
	bool reload();
	
	void setFontsDirectory( std::string aDir );
	
	std::string toString(int nlevel = 0) override;
	
	bool getTransformFromSvgMatrix( std::string aStr, glm::vec2& apos, float& scaleX, float& scaleY, float& arotation );
	
	const ofRectangle getBounds();
	const ofRectangle getViewbox();
	
	const int getTotalLayers();
	
	virtual void drawDebug();
	
protected:
	std::string fontsDirectory = "";
	std::string folderPath, svgPath;
	ofRectangle viewbox;
	ofRectangle bounds;
	void validateXmlSvgRoot( ofXml& aRootSvgNode );
	std::string cleanString( std::string aStr, std::string aReplace );
	void _parseXmlNode( ofXml& aParentNode, std::vector< std::shared_ptr<Element> >& aElements );
	bool _addElementFromXmlNode( ofXml& tnode, std::vector< std::shared_ptr<Element> >& aElements );
	
	void _parsePolylinePolygon( ofXml& tnode, std::shared_ptr<Path> aSvgPath );
	void _parsePath( ofXml& tnode, std::shared_ptr<Path> aSvgPath );
	
	CssClass _parseStyle( ofXml& tnode );
	void _applyStyleToElement( ofXml& tnode, std::shared_ptr<Element> aEle );
	void _applyStyleToPath( ofXml& tnode, std::shared_ptr<Path> aSvgPath );
	void _applyStyleToPath( CssClass& aclass, std::shared_ptr<Path> aSvgPath );
	void _applyStyleToText( ofXml& tnode, std::shared_ptr<Text::TextSpan> aTextSpan );
	void _applyStyleToText( CssClass& aclass, std::shared_ptr<Text::TextSpan> aTextSpan );
	
	glm::vec2 _parseMatrixString(const std::string& input, const std::string& aprefix );
	
	std::shared_ptr<Text::TextSpan> getTextSpanFromXmlNode( ofXml& anode );
	int mCurrentLayer = 0;
	
	ofx::svg::CssStyleSheet mSvgCss;
	
	std::shared_ptr<ofx::svg::CssClass> mCurrentSvgCss;
	
	std::vector< std::shared_ptr<Element> > mDefElements;
	
	// just used for debugging
	std::vector<glm::vec3> mCPoints;
	std::vector<glm::vec3> mCenterPoints;
};

}







