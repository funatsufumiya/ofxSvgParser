//
//  ofxSvg2Css.h
//  Example
//
//  Created by Nick Hardeman on 8/22/24.
//

#pragma once
#include <unordered_map>
#include "ofColor.h"

namespace ofx::svg {
class CssClass {
public:
	
	class Property {
	public:
		std::string srcString;
		std::optional<float> fvalue;
		std::optional<int> ivalue;
		std::optional<std::string> svalue;
		std::optional<ofColor> cvalue;
		
//		bool bInPixels = false;
//		bool bHasHash = false;
	};
	
	std::unordered_map<std::string, Property> properties;
	std::string name = "default";
	
	static bool sIsNone( const std::string& astr );
	static ofColor sGetColor(const std::string& astr);
	static float sGetFloat(const std::string& astr);
	
	bool addProperties( std::string aPropertiesString );
	bool addProperty( std::string aPropString );
	bool addProperty( std::string aName, std::string avalue );
	bool addProperty( const std::string& aName, const Property& aprop );
	
	bool hasProperty( const std::string& akey );
	Property& getProperty( const std::string& akey );
	bool isNone(const std::string& akey);
	bool hasAndIsNone(const std::string& akey);
	
	std::string getValue(const std::string& akey, const std::string& adefault);
	int getIntValue(const std::string& akey, const int& adefault);
	float getFloatValue(const std::string& akey, const float& adefault);
	ofColor getColor(const std::string& akey);
	
	std::string toString();
	
protected:
	Property dummyProp;
};

class CssStyleSheet {
public:
	
	bool parse( std::string aCssString );
	void clear();
	
	CssClass& addClass( std::string aname );
	bool hasClass( const std::string& aname );
	CssClass& getClass( const std::string& aname );
	
	std::unordered_map<std::string, CssClass> classes;
	
	std::string toString();
	
protected:
	CssClass dummyClass;
};
}
