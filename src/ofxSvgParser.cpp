//
//  ofxSvgParser.cpp
//
//  Created by Nick Hardeman on 8/31/24.
//

#include "ofxSvgParser.h"
#include "ofUtils.h"
#include <regex>
#include "ofGraphics.h"

using namespace ofx::svg;
using std::string;
using std::vector;
using std::shared_ptr;

//--------------------------------------------------------------
bool Parser::load( of::filesystem::path aPathToSvg ) {
    mChildren.clear();
	mDefElements.clear();
    mCurrentLayer = 0;
	mCurrentSvgCss.reset();
	mSvgCss.clear();
	mCPoints.clear();
	mCenterPoints.clear();
    
    ofFile mainXmlFile( aPathToSvg, ofFile::ReadOnly );
    ofBuffer tMainXmlBuffer( mainXmlFile );
    
    svgPath     = aPathToSvg;
    folderPath  = ofFilePath::getEnclosingDirectory( aPathToSvg, false );
    
    ofXml xml;
    if( !xml.load(tMainXmlBuffer )) {
		ofLogWarning(moduleName()) << " unable to load svg from " << aPathToSvg;
        return false;
    }
    
    
    if( xml ) {
        ofXml svgNode = xml.getFirstChild();
        
        validateXmlSvgRoot( svgNode );
        
        ofXml::Attribute viewBoxAttr = svgNode.getAttribute("viewBox");
        if(svgNode) {
            bounds.x        = ofToFloat( cleanString( svgNode.getAttribute("x").getValue(), "px") );
            bounds.y        = ofToFloat( cleanString( svgNode.getAttribute("y").getValue(), "px" ));
            bounds.width    = ofToFloat( cleanString( svgNode.getAttribute("width").getValue(), "px" ));
            bounds.height   = ofToFloat( cleanString( svgNode.getAttribute("height").getValue(), "px" ));
            viewbox = bounds;
        }
        
        if( viewBoxAttr ) {
            string tboxstr = viewBoxAttr.getValue();
            vector< string > tvals = ofSplitString( tboxstr, " " );
            if( tvals.size() == 4 ) {
                viewbox.x = ofToFloat(tvals[0] );
                viewbox.y = ofToFloat( tvals[1] );
                viewbox.width = ofToFloat( tvals[2] );
                viewbox.height = ofToFloat( tvals[3] );
            }
        }
		
		if(svgNode) {
			ofLogVerbose(moduleName()) << svgNode.findFirst("style").toString() << "  bounds: " << bounds;
		} else {
			ofLogVerbose( moduleName() ) << __FUNCTION__ << " : NO svgNode: ";
		}
		
		
		ofXml styleXmlNode = svgNode.findFirst("//style");
		if( styleXmlNode ) {
			ofLogVerbose(moduleName()) << __FUNCTION__ << " : STYLE NODE" << styleXmlNode.getAttribute("type").getValue() << " string: " << styleXmlNode.getValue();
			
			mSvgCss.parse(styleXmlNode.getValue());
			
			ofLogVerbose(moduleName()) << "-----------------------------";
			ofLogVerbose() << mSvgCss.toString();
			ofLogVerbose(moduleName()) << "-----------------------------";
 		} else {
			ofLogVerbose(moduleName()) << __FUNCTION__ << " : NO STYLE NODE";
		}
        
		
		// the defs are added in the _parseXmlNode function //
		_parseXmlNode( svgNode, mChildren );
		
		ofLogVerbose(moduleName()) << " number of defs elements: " << mDefElements.size();
    }
    
    return true;
}

//--------------------------------------------------------------
bool Parser::reload() {
    if( svgPath.empty() ) {
        ofLogError(moduleName()) << __FUNCTION__ << " : svg path is empty, please call load with file path before calling reload";
        return false;
    }
    return load( svgPath );
}

//--------------------------------------------------------------
const int Parser::getTotalLayers(){
	return mCurrentLayer;
}

//--------------------------------------------------------------
void Parser::setFontsDirectory( string aDir ) {
    fontsDirectory = aDir;
    if( fontsDirectory.back() != '/' ) {
        fontsDirectory += '/';
    }
}

//--------------------------------------------------------------
string Parser::toString(int nlevel) {
    string tstr = "";
    if( mChildren.size() ) {
        for( std::size_t i = 0; i < mChildren.size(); i++ ) {
            tstr += mChildren[i]->toString( nlevel );
        }
    }
    return tstr;
}

//--------------------------------------------------------------
void Parser::validateXmlSvgRoot( ofXml& aRootSvgNode ) {
    // if there is no width and height set in the svg base node, svg tiny no likey //
    if(aRootSvgNode) {
        // check for x, y, width and height //
        {
            auto xattr = aRootSvgNode.getAttribute("x");
            if( !xattr ) {
                auto nxattr = aRootSvgNode.appendAttribute("x");
                if(nxattr) nxattr.set("0px");
            }
        }
        {
            auto yattr = aRootSvgNode.getAttribute("y");
            if( !yattr ) {
                auto yattr = aRootSvgNode.appendAttribute("y");
                if( yattr ) yattr.set("0px");
            }
        }
        
        auto wattr = aRootSvgNode.getAttribute("width");
        auto hattr = aRootSvgNode.getAttribute("height");
        
        if( !wattr || !hattr ) {
            ofXml::Attribute viewBoxAttr = aRootSvgNode.getAttribute("viewBox");
            if( viewBoxAttr ) {
                string tboxstr = viewBoxAttr.getValue();
                vector< string > tvals = ofSplitString( tboxstr, " " );
                if( tvals.size() >= 4 ) {
                    if( !wattr ) {
                        auto nwattr = aRootSvgNode.appendAttribute("width");
                        if(nwattr) nwattr.set( ofToString(tvals[2])+"px" );
                    }
                    
                    if( !hattr ) {
                        auto nhattr = aRootSvgNode.appendAttribute("height");
                        if(nhattr) nhattr.set( ofToString(tvals[3])+"px" );
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------
string Parser::cleanString( string aStr, string aReplace ) {
    ofStringReplace( aStr, aReplace, "");
    return aStr;
}

//--------------------------------------------------------------
void Parser::_parseXmlNode( ofXml& aParentNode, vector< shared_ptr<Element> >& aElements ) {
    
    auto kids = aParentNode.getChildren();
    for( auto& kid : kids ) {
		if( kid.getName() == "g" ) {
			auto fkid = kid.getFirstChild();
			if( fkid ) {
				mCurrentSvgCss.reset();
				auto tgroup = std::make_shared<Group>();
				tgroup->layer = mCurrentLayer += 1.0;
				auto idattr = kid.getAttribute("id");
				if( idattr ) {
					tgroup->name = idattr.getValue();
				}
				
				mCurrentSvgCss = std::make_shared<ofx::svg::CssClass>( _parseStyle(kid) );
				
				aElements.push_back( tgroup );
				_parseXmlNode( kid, tgroup->getChildren() );
			}
		} else if( kid.getName() == "defs") {
			ofLogVerbose(moduleName()) << __FUNCTION__ << " found a defs node.";
			_parseXmlNode(kid, mDefElements );
        } else {
            
            bool bAddOk = _addElementFromXmlNode( kid, aElements );
//            cout << "----------------------------------" << endl;
//            cout << kid.getName() << " kid: " << kid.getAttribute("id").getValue() << " out xml: " << txml.toString() << endl;
        }
    }
}

//--------------------------------------------------------------
bool Parser::_addElementFromXmlNode( ofXml& tnode, vector< shared_ptr<Element> >& aElements ) {
    shared_ptr<Element> telement;
	
	if( tnode.getName() == "use") {
		if( auto hrefAtt = tnode.getAttribute("xlink:href")) {
			ofLogVerbose(moduleName()) << "found a use node with href " << hrefAtt.getValue();
			std::string href = hrefAtt.getValue();
			if( href.size() > 1 && href[0] == '#' ) {
				// try to find by id
				href = href.substr(1, std::string::npos);
				ofLogVerbose(moduleName()) << "going to look for href " << href;
				for( auto & def : mDefElements ) {
					if( def->name == href ) {
						if( def->getType() == ofx::svg::TYPE_RECTANGLE ) {
							auto drect = std::dynamic_pointer_cast<ofx::svg::Rectangle>(def);
							auto nrect = std::make_shared<ofx::svg::Rectangle>( *drect );
							telement = nrect;
						} else if( def->getType() == ofx::svg::TYPE_IMAGE ) {
							auto dimg = std::dynamic_pointer_cast<ofx::svg::Image>(def);
							auto nimg = std::make_shared<ofx::svg::Image>( *dimg );
							ofLogVerbose(moduleName()) << "created an image node with filepath: " << nimg->getFilePath();
							telement = nimg;
						} else if( def->getType() == ofx::svg::TYPE_ELLIPSE ) {
							auto dell= std::dynamic_pointer_cast<ofx::svg::Ellipse>(def);
							auto nell = std::make_shared<ofx::svg::Ellipse>( *dell );
							telement = nell;
						} else if( def->getType() == ofx::svg::TYPE_CIRCLE ) {
							auto dcir= std::dynamic_pointer_cast<ofx::svg::Circle>(def);
							auto ncir = std::make_shared<ofx::svg::Circle>( *dcir );
							telement = ncir;
						} else if( def->getType() == ofx::svg::TYPE_PATH ) {
							auto dpat= std::dynamic_pointer_cast<ofx::svg::Path>(def);
							auto npat = std::make_shared<ofx::svg::Path>( *dpat );
							telement = npat;
						} else if( def->getType() == ofx::svg::TYPE_TEXT ) {
							auto dtex = std::dynamic_pointer_cast<ofx::svg::Text>(def);
							auto ntex = std::make_shared<ofx::svg::Text>( *dtex );
							telement = ntex;
						} else {
							ofLogWarning("Parser") << "could not find type for def : " << def->name;
						}
						break;
					}
				}
			} else {
				ofLogWarning(moduleName()) << "could not parse use node with href : " << href;
			}
		} else {
			ofLogWarning(moduleName()) << "found a use node but no href!";
		}
	} else if( tnode.getName() == "image" ) {
        auto image = std::make_shared<Image>();
        auto wattr = tnode.getAttribute("width");
        if(wattr) image->width  = wattr.getFloatValue();
        auto hattr = tnode.getAttribute("height");
        if(hattr) image->height = hattr.getFloatValue();
        auto xlinkAttr = tnode.getAttribute("xlink:href");
        if( xlinkAttr ) {
            image->filepath = folderPath+xlinkAttr.getValue();
        }
        telement = image;
        
    } else if( tnode.getName() == "ellipse" ) {
        auto ellipse = std::make_shared<Ellipse>();
        auto cxAttr = tnode.getAttribute("cx");
        if(cxAttr) ellipse->pos.x = cxAttr.getFloatValue();
        auto cyAttr = tnode.getAttribute("cy");
        if(cyAttr) ellipse->pos.y = cyAttr.getFloatValue();
        
        auto rxAttr = tnode.getAttribute( "rx" );
        if(rxAttr) ellipse->radiusX = rxAttr.getFloatValue();
        auto ryAttr = tnode.getAttribute( "ry" );
        if(ryAttr) ellipse->radiusY = ryAttr.getFloatValue();
		
		// make local so we can apply transform later in the function
		ellipse->path.ellipse({0.f,0.f}, ellipse->radiusX, ellipse->radiusY );
		
		_applyStyleToPath( tnode, ellipse );
        
        telement = ellipse;
	} else if( tnode.getName() == "circle" ) {
		auto circle = std::make_shared<Circle>();
		auto cxAttr = tnode.getAttribute("cx");
		if(cxAttr) circle->pos.x = cxAttr.getFloatValue();
		auto cyAttr = tnode.getAttribute("cy");
		if(cyAttr) circle->pos.y = cyAttr.getFloatValue();
		
		auto rAttr = tnode.getAttribute( "r" );
		if(rAttr) circle->radius = rAttr.getFloatValue();
		
		// make local so we can apply transform later in the function
		// position is from the top left
		circle->path.circle({0.f,0.f}, circle->radius );
		
		_applyStyleToPath( tnode, circle );
		
		telement = circle;
		
	} else if( tnode.getName() == "line" ) {
		auto telePath = std::make_shared<Path>();
		
		glm::vec3 p1 = {0.f, 0.f, 0.f};
		glm::vec3 p2 = {0.f, 0.f, 0.f};
		auto x1Attr = tnode.getAttribute("x1");
		if(x1Attr) p1.x = x1Attr.getFloatValue();
		auto y1Attr = tnode.getAttribute("y1");
		if(y1Attr) p1.y = y1Attr.getFloatValue();
		
		auto x2Attr = tnode.getAttribute("x2");
		if(x2Attr) p2.x = x2Attr.getFloatValue();
		auto y2Attr = tnode.getAttribute("y2");
		if(y2Attr) p2.y = y2Attr.getFloatValue();
		
		// set the colors and stroke width, etc.
		telePath->path.clear();
		telePath->path.moveTo(p1);
		telePath->path.lineTo(p2);
		
		_applyStyleToPath( tnode, telePath );
		
		telement = telePath;
        
	} else if(tnode.getName() == "polyline" || tnode.getName() == "polygon") {
		auto tpath = std::make_shared<Path>();
		_parsePolylinePolygon(tnode, tpath);
		_applyStyleToPath( tnode, tpath );
		telement = tpath;
	} else if( tnode.getName() == "path" ) {
		auto tpath = std::make_shared<Path>();
		_parsePath( tnode, tpath );
		_applyStyleToPath( tnode, tpath );
		telement = tpath;
    } else if( tnode.getName() == "rect" ) {
        auto rect = std::make_shared<Rectangle>();
        auto xattr = tnode.getAttribute("x");
        if(xattr) rect->rectangle.x       = xattr.getFloatValue();
        auto yattr = tnode.getAttribute("y");
        if(yattr) rect->rectangle.y       = yattr.getFloatValue();
        auto wattr = tnode.getAttribute("width");
        if(wattr) rect->rectangle.width   = wattr.getFloatValue();
        auto hattr = tnode.getAttribute("height");
        if(hattr) rect->rectangle.height  = hattr.getFloatValue();
        rect->pos.x = rect->rectangle.x;
        rect->pos.y = rect->rectangle.y;
		
		auto rxAttr = tnode.getAttribute("rx");
		auto ryAttr = tnode.getAttribute("ry");
		
		// make local so we can apply transform later in the function
		if( !CssClass::sIsNone(rxAttr.getValue()) || !CssClass::sIsNone(ryAttr.getValue())) {
			rect->path.rectRounded(0.f, 0.f, rect->rectangle.getWidth(), rect->rectangle.getHeight(),
									std::max(CssClass::sGetFloat(rxAttr.getValue()),
											CssClass::sGetFloat(ryAttr.getValue()))
								   );
		} else {
			rect->path.rectangle(0.f, 0.f, rect->getWidth(), rect->getHeight());
		}
        
        telement = rect;
        		
		_applyStyleToPath( tnode, rect );
        
        // this shouldn't be drawn at all, may be a rect that for some reason is generated
        // by text blocks //
        if( !rect->isFilled() && !rect->hasStroke() ) {
			telement->setVisible(false);
        }
        
    } else if( tnode.getName() == "text" ) {
        auto text = std::make_shared<Text>();
        telement = text;
//		std::cout << "has kids: " << tnode.getFirstChild() << " node value: " << tnode.getValue() << std::endl;
        if( tnode.getFirstChild() ) {
            
            auto kids = tnode.getChildren();
            for( auto& kid : kids ) {
                if(kid) {
                    if( kid.getName() == "tspan" ) {
                        text->textSpans.push_back( getTextSpanFromXmlNode( kid ) );
                    }
                }
            }
            
            // this may not be a text block or it may have no text //
            if( text->textSpans.size() == 0 ) {
				text->textSpans.push_back( getTextSpanFromXmlNode( tnode ) );
            }
        }
        
        string tempFolderPath = folderPath;
        if( tempFolderPath.back() != '/' ) {
            tempFolderPath += '/';
        }
        if( ofDirectory::doesDirectoryExist( tempFolderPath+"fonts/" )) {
            text->setFontDirectory( tempFolderPath+"fonts/" );
        }
        if( fontsDirectory != "" ) {
            if( ofDirectory::doesDirectoryExist(fontsDirectory)) {
                text->setFontDirectory( fontsDirectory );
            }
        }
        
    } else if( tnode.getName() == "g" ) {
		
    }
    
    if( !telement ) {
        return false;
    }
    
    auto idAttr = tnode.getAttribute("id");
    if( idAttr ) {
        telement->name = idAttr.getValue();
    }
    
    if( telement->getType() == TYPE_RECTANGLE || telement->getType() == TYPE_IMAGE || telement->getType() == TYPE_TEXT || telement->getType() == TYPE_CIRCLE || telement->getType() == TYPE_ELLIPSE ) {
        auto transAttr = tnode.getAttribute("transform");
        if( transAttr ) {
            getTransformFromSvgMatrix( transAttr.getValue(), telement->pos, telement->scale.x, telement->scale.y, telement->rotation );
        }
		
		std::vector<SvgType> typesToApplyTransformToPath = {
			TYPE_RECTANGLE,
			TYPE_CIRCLE,
			TYPE_ELLIPSE
		};
		
		bool bApplyTransformToPath = false;
		for( auto & etype : typesToApplyTransformToPath ) {
			if( etype == telement->getType() ) {
				bApplyTransformToPath = true;
				break;
			}
		}
		
		if( bApplyTransformToPath ) {
			auto epath = std::dynamic_pointer_cast<Path>( telement );
			auto outlines = epath->path.getOutline();
			auto transform = epath->getTransformMatrix();
			for( auto& outline : outlines ) {
				for( auto& v : outline ) {
					v = transform * glm::vec4(v, 1.0f);
				}
			}
			// now we have new outlines, what do we do?
			epath->path.clear();
			bool bFirstOne = true;
			for( auto& outline : outlines ) {
				for( auto& v : outline ) {
					if(bFirstOne) {
						bFirstOne = false;
						epath->path.moveTo(v);
					} else {
						epath->path.lineTo(v);
					}
				}
				if( outline.isClosed() ) {
					epath->path.close();
				}
			}
		}
    }
    
    if( telement->getType() == TYPE_TEXT ) {
        auto text = std::dynamic_pointer_cast<Text>( telement );
        text->ogPos = text->pos;
        text->create();
    }
	
	_applyStyleToElement(tnode, telement);
        
	telement->layer = mCurrentLayer += 1.0;
    aElements.push_back( telement );
    return true;
}

std::vector<glm::vec3> parsePoints(const std::string& input) {
	std::vector<glm::vec3> points;
	std::regex regex("[-]?\\d*\\.?\\d+");  // Matches positive/negative floats
	std::sregex_iterator begin(input.begin(), input.end(), regex), end;
	
	std::vector<float> values;
	
	// Extract all floating-point values using regex
	for (std::sregex_iterator i = begin; i != end; ++i) {
		try {
			values.push_back(std::stof((*i).str()));
		} catch (const std::invalid_argument&) {
			std::cerr << "Invalid number found: " << (*i).str() << std::endl;
		}
	}
	
	// Create vec2 pairs from the values
	for (size_t i = 0; i < values.size(); i += 2) {
		if (i + 1 < values.size()) {
			glm::vec3 point(values[i], values[i + 1], 0.f);
			points.push_back(point);
		}
	}
	
	if( values.size() == 1 && points.size() < 1) {
		glm::vec3 point(values[0], values[0], 0.f);
		points.push_back(point);
	}
	
	return points;
}


//----------------------------------------------------
auto _parseStrCoordsFunc = [](const std::string& apointsStr ) -> std::vector< glm::vec3 > {
	std::vector<glm::vec3> coordinates;
	unsigned int num = 0;
	
	std::regex regex_pattern(R"((-?\d*\.?\d+),(-?\d*\.?\d+)|(-?\d*\.?\d+))");
	std::smatch match;
	
	std::string::const_iterator search_start(apointsStr.cbegin());
	
	while (std::regex_search(search_start, apointsStr.cend(), match, regex_pattern)) {
		if (match[1].matched && match[2].matched) {
			// Found a comma-separated pair (X, Y)
			float x = std::stof(match[1]);  // First captured group
			float y = std::stof(match[2]);  // Second captured group
			coordinates.push_back({x, y, 0.f});
		} else if (match[3].matched) {
			// Found a single number (X), with Y assumed to be 0
			float x = std::stof(match[3]);  // Third captured group
			coordinates.push_back({x, 0.f, 0.f});  // Y is assumed to be 0
		}
		
		search_start = match.suffix().first;  // Move the search position forward
		
		num++;
		if( num > 999999 ) {
			break;
		}
	}
	
	return coordinates;
};

//----------------------------------------------------
int _getWindingOrderOnArc( glm::vec3& aStartPos, glm::vec3& aCenterPos, glm::vec3& aendPos ) {
	glm::vec3 sdiff = glm::normalize(aStartPos - aCenterPos);
	glm::vec3 ediff = glm::normalize(aendPos - aCenterPos);
	float tcross = sdiff.x * ediff.y - sdiff.y * ediff.x;
//	ofLogNotice("_getWindingOrderOnArc") << "tcross is " << tcross;
	if( tcross > 0.0f ) {
		// clockwise
		return 1;
	} else if( tcross < 0.0f ) {
		// counter clockwise
		return -1;
	}
	// co-linear
	return 0;
	
}

//----------------------------------------------------
// Function to find the center of the elliptical arc from SVG arc parameters
glm::vec2 findArcCenter(glm::vec2 start, glm::vec2 end, double rx, double ry, double x_axis_rotation, bool large_arc_flag, bool sweep_flag) {
	// Convert the rotation to radians
	double phi = glm::radians(x_axis_rotation);
	double cos_phi = cos(phi);
	double sin_phi = sin(phi);
	
	// Step 1: Compute (x1', y1') - the coordinates of the start point in the transformed coordinate system
	glm::vec2 diff = (start - end) / 2.0f;
	glm::vec2 p1_prime(cos_phi * diff.x + sin_phi * diff.y, -sin_phi * diff.x + cos_phi * diff.y);
	
	// Step 2: Correct radii if necessary
	double p1_prime_x_sq = p1_prime.x * p1_prime.x;
	double p1_prime_y_sq = p1_prime.y * p1_prime.y;
	double rx_sq = rx * rx;
	double ry_sq = ry * ry;
	double radii_check = p1_prime_x_sq / rx_sq + p1_prime_y_sq / ry_sq;
	if (radii_check > 1) {
		// Scale radii to ensure the arc can fit between the two points
		double scale = std::sqrt(radii_check);
		rx *= scale;
		ry *= scale;
		rx_sq = rx * rx;
		ry_sq = ry * ry;
	}
	
	// Step 3: Compute (cx', cy') - the center point in the transformed coordinate system
	double factor_numerator = rx_sq * ry_sq - rx_sq * p1_prime_y_sq - ry_sq * p1_prime_x_sq;
	double factor_denominator = rx_sq * p1_prime_y_sq + ry_sq * p1_prime_x_sq;
	if (factor_numerator < 0) {
		factor_numerator = 0; // Precision error correction to avoid sqrt of negative numbers
	}
	
	double factor = std::sqrt(factor_numerator / factor_denominator);
	if (large_arc_flag == sweep_flag) {
		factor = -factor;
	}
	
	glm::vec2 center_prime(factor * rx * p1_prime.y / ry, factor * -ry * p1_prime.x / rx);
	
	// Step 4: Compute the center point in the original coordinate system
	glm::vec2 center(
					 cos_phi * center_prime.x - sin_phi * center_prime.y + (start.x + end.x) / 2.0,
					 sin_phi * center_prime.x + cos_phi * center_prime.y + (start.y + end.y) / 2.0
					 );
	
	return center;
}


//--------------------------------------------------------------
void Parser::_parsePolylinePolygon( ofXml& tnode, std::shared_ptr<Path> aSvgPath ) {
	auto pointsAttr = tnode.getAttribute("points");
	if( !pointsAttr ) {
		ofLogWarning(moduleName()) << __FUNCTION__ << " polyline or polygon does not have a points attriubute.";
		return;
	}
	
	if( pointsAttr.getValue().empty() ) {
		ofLogWarning(moduleName()) << __FUNCTION__ << " polyline or polygon does not have points.";
		return;
	}
	
	auto points = parsePoints(pointsAttr.getValue());
	std::size_t numPoints = points.size();
	for( std::size_t i = 0; i < numPoints; i++ ) {
		if( i == 0 ) {
			aSvgPath->path.moveTo(points[i]);
		} else {
			aSvgPath->path.lineTo(points[i]);
		}
	}
	if( numPoints > 2 ) {
		if(tnode.getName() == "polygon" ) {
			aSvgPath->path.close();
		}
	}
}
// reference: https://www.w3.org/TR/SVG2/paths.html#PathData
//--------------------------------------------------------------
void Parser::_parsePath( ofXml& tnode, std::shared_ptr<Path> aSvgPath ) {
	aSvgPath->path.clear();
	
	auto dattr = tnode.getAttribute("d");
	if( !dattr ) {
		ofLogWarning(moduleName()) << __FUNCTION__ << " path node does not have d attriubute.";
		return;
	}
	
	std::vector<unsigned char> splitChars = {
		'M', 'm', // move to
		'V', 'v', // vertical line
		'H', 'h', // horizontal line
		'L','l', // line
		'z','Z', // close path
		'c','C','s','S', // cubic bézier
		'Q', 'q', 'T', 't', // quadratic bézier
		'A', 'a' // elliptical arc
	};
	std::string ostring = dattr.getValue();
//	ofLogNotice(moduleName()) << __FUNCTION__ << " dattr: " << ostring;
	
	if( ostring.empty() ) {
		ofLogError(moduleName()) << __FUNCTION__ << " there is no data in the d string.";
		return;
	}
	
	std::size_t index = 0;
	if( ostring[index] != 'm' && ostring[index] != 'M' ) {
		ofLogWarning(moduleName()) << __FUNCTION__ << " first char is not a m or M, ostring[index]: " << ostring[index];
		return;
	}
	
	glm::vec3 currentPos = {0.f, 0.f, 0.f};
	glm::vec3 secondControlPoint = currentPos;
	glm::vec3 qControlPoint = currentPos;
	
	auto convertToAbsolute = [](bool aBRelative, glm::vec3& aCurrentPos, std::vector<glm::vec3>& aposes) -> glm::vec3 {
		for(auto& apos : aposes ) {
			if( aBRelative ) {
				apos += aCurrentPos;
			}
		}
		if( aposes.size() > 0 ) {
			aCurrentPos = aposes.back();
		}
		return aCurrentPos;
	};
	
	auto convertToAbsolute2 = [](bool aBRelative, glm::vec3& aCurrentPos, std::vector<glm::vec3>& aposes) -> glm::vec3 {
		for( std::size_t k = 0; k < aposes.size(); k+= 1 ) {
			if( aBRelative ) {
				aposes[k] += aCurrentPos;
			}
			if( k > 0 && k % 2 == 1 ) {
				aCurrentPos = aposes[k];
			}
		}
		return aCurrentPos;
	};
	
	auto convertToAbsolute3 = [](bool aBRelative, glm::vec3& aCurrentPos, std::vector<glm::vec3>& aposes) -> glm::vec3 {
		for( std::size_t k = 0; k < aposes.size(); k+= 1 ) {
			if( aBRelative ) {
				aposes[k] += aCurrentPos;
			}
			
			if( k > 0 && k % 3 == 2 ) {
				aCurrentPos = aposes[k];
			}
			
		}
		return aCurrentPos;
	};
	
	
	
	aSvgPath->path.clear();
	
	unsigned int justInCase = 0;
//	std::vector<ofPath::Command> commands;
	bool breakMe = false;
	while( index < ostring.size() && !breakMe && justInCase < 9999) {
		// figure out what we have here .
		auto cchar = ostring[index];
		// check for valid character //
		bool bFoundValidChar = false;
		for( auto& sc : splitChars ) {
			if( sc == cchar ) {
				bFoundValidChar = true;
				break;
			}
		}
		if( !bFoundValidChar ) {
			breakMe = true;
			break;
		}
		
//		ofLogNotice(moduleName()) << " o : ["<< ostring[index] <<"]";
		
		// up to next valid character //
		std::string currentString;
		bool bFoundValidNextChar = false;
		auto pos = index+1;
		if( pos >= ostring.size() ) {
			break;
		}
		
		bFoundValidChar = false;
		for( pos = index+1; pos < ostring.size(); pos++ ) {
			for( auto& sc : splitChars ) {
				if( sc == ostring[pos] ) {
					bFoundValidChar = true;
					break;
				}
			}
			if( bFoundValidChar ) {
				break;
			}
			currentString.push_back(ostring[pos]);
		}
		
		index += currentString.size()+1;
		
		
		if( currentString.empty() ) {
			break;
		}
		
		
//		ofLogNotice(moduleName()) << "["<<cchar<<"]: " << currentString;
		
		bool bRelative = false;
		std::vector<glm::vec3> npositions= {glm::vec3(0.f, 0.f, 0.f)};
		std::optional<ofPath::Command::Type> ctype;
		
		// check if we are looking for a position
		if( cchar == 'm' || cchar == 'M' ) {
			
			npositions = _parseStrCoordsFunc(currentString);
			if( npositions.size() > 0 ) {
				if( cchar == 'm' ) {
					bRelative = true;
				}
//				secondControlPoint = npositions[0];
				ctype = ofPath::Command::moveTo;
			}
		} else if( cchar == 'v' || cchar == 'V' ) {
			if( cchar == 'v' ) {
				bRelative = true;
			}
			npositions[0].x = 0.f;
			npositions[0].y = ofToFloat(currentString);
			ctype = ofPath::Command::lineTo;
		} else if( cchar == 'H' || cchar == 'h' ) {
			if( cchar == 'h' ) {
				bRelative = true;
			}
			npositions[0].x = ofToFloat(currentString);
			npositions[0].y = 0.f;
			ctype = ofPath::Command::lineTo;
		} else if( cchar == 'L' || cchar == 'l' ) {
			if( cchar == 'l' ) {
				bRelative = true;
			}
			npositions = parsePoints(currentString);
//			for( auto& np : npositions ) {
//				ofLogNotice(moduleName()) << cchar << " line to: " << np;
//			}
			ctype = ofPath::Command::lineTo;
		} else if( cchar == 'z' || cchar == 'Z' ) {
			ctype = ofPath::Command::close;
		} else if( cchar == 'c' || cchar == 'C' || cchar == 'S' || cchar == 's' ) {
			if( cchar == 'c' || cchar == 's') {
				bRelative = true;
			}
			ctype = ofPath::Command::bezierTo;
			npositions = parsePoints(currentString);
//			for( auto& np : npositions ) {
//				ofLogNotice(moduleName()) << cchar << " bezier to: " << np;
//			}
		} else if( cchar == 'Q' || cchar == 'q' || cchar == 'T' || cchar == 't' ) {
			if( cchar == 'q' ) {
				bRelative = true;
			}
			
			ctype = ofPath::Command::quadBezierTo;
			npositions = parsePoints(currentString);
			
//			for( auto& np : npositions ) {
//				ofLogNotice(moduleName()) << " Quad bezier to: " << np;
//			}
		} else if(cchar == 'a' || cchar == 'A' ) {
			if( cchar == 'a' ) {
				bRelative = true;
			}
			ctype = ofPath::Command::arc;
			npositions = _parseStrCoordsFunc(currentString);
			
//			for( auto& np : npositions ) {
//				ofLogNotice(moduleName()) << " arc parsed positions: " << np;
//			}
		}
		
		if( ctype.has_value() ) {
			auto prevPos = currentPos;
			
			auto commandT = ctype.value();
			
			if( commandT == ofPath::Command::arc ) {
				if( npositions.size() == 4 ) {
					std::vector<glm::vec3> tpositions = {npositions[3]};
					currentPos = convertToAbsolute(bRelative, currentPos, tpositions );
					npositions[3] = tpositions[0];
				} else {
					ofLogWarning("ofx::svg::Parser") << "invalid number of arc commands.";
				}
			} else if( commandT == ofPath::Command::bezierTo ) {
				if( cchar == 'S' || cchar == 's' ) {
					currentPos = convertToAbsolute2(bRelative, currentPos, npositions );
				} else {
					currentPos = convertToAbsolute3(bRelative, currentPos, npositions );
				}
//			} else if( commandT == ofPath::Command::quadBezierTo ) {
				// TODO: Check quad bezier for poly bezier like cubic bezier
				
			} else {
				currentPos = convertToAbsolute(bRelative, currentPos, npositions );
			}
			
			if( commandT != ofPath::Command::bezierTo ) {
				secondControlPoint = currentPos;
			}
			if( commandT != ofPath::Command::quadBezierTo ) {
				qControlPoint = currentPos;
			}
			
			if( commandT == ofPath::Command::moveTo ) {
				aSvgPath->path.moveTo(npositions[0]);
			} else if( commandT == ofPath::Command::lineTo ) {
				aSvgPath->path.lineTo(npositions[0]);
			} else if( commandT == ofPath::Command::close ) {
				aSvgPath->path.close();
			} else if( commandT == ofPath::Command::bezierTo ) {
				
				if( cchar == 'S' || cchar == 's' ) {
					// these can come in as multiple sets of points //
					std::vector<glm::vec3> ppositions;// = npositions;
					auto tppos = prevPos;
					for( std::size_t i = 0; i < npositions.size(); i += 2 ) {
						auto cp2 = (secondControlPoint - tppos) * -1.f;
						cp2 += tppos;
						ppositions.push_back( cp2 );
						ppositions.push_back(npositions[i+0]);
						ppositions.push_back(npositions[i+1]);
						tppos = npositions[i+1];
						secondControlPoint = npositions[i+0];
					}
					
					npositions = ppositions;
					
//					if( npositions.size() == 2 ) {
//						auto cp2 = (secondControlPoint - prevPos) * -1.f;
//						cp2 += prevPos;
//						npositions.insert(npositions.begin(), cp2 );
//					}
				}
				
				auto tcpos = prevPos;
				
				for( std::size_t k = 0; k < npositions.size(); k +=3 ) {
					aSvgPath->path.bezierTo(npositions[k+0], npositions[k+1], npositions[k+2]);
					secondControlPoint = npositions[k+1];
					
					mCPoints.push_back(prevPos);
					mCPoints.push_back(npositions[k+0]);
					mCPoints.push_back(npositions[k+1]);
					tcpos = npositions[k+2];
					
//					mCPoints.push_back(npositions[k+0]);
//					mCPoints.push_back(npositions[k+1]);
//					mCenterPoints.push_back(npositions[k+2]);
				}
				
//				mCPoints.insert( mCPoints.end(), npositions.begin(), npositions.end() );
				
//				if( npositions.size() == 3 ) {
//					aSvgPath->path.bezierTo(npositions[0], npositions[1], npositions[2]);
//				}
//				
//				secondControlPoint = npositions[1];
			} else if( commandT == ofPath::Command::quadBezierTo ) {
				if( cchar == 'T' || cchar == 't' ) {
					if( npositions.size() == 1 ) {
						auto cp2 = (qControlPoint - prevPos) * -1.f;
						cp2 += prevPos;
						npositions.insert(npositions.begin(), cp2 );
					}
				}
				
				if( npositions.size() == 2 ) {
					aSvgPath->path.quadBezierTo(prevPos, npositions[0], npositions[1] );
				}
				qControlPoint = npositions[0];
			} else if( commandT == ofPath::Command::arc ) {
				if( npositions.size() == 4 ) {
					// first point is rx, ry
					// second point x value is x-axis rotation
					// third point x value is large-arc-flag, y value is sweep-flag
					// fourth point is x and y: When a relative a command is used, the end point of the arc is (cpx + x, cpy + y).
					glm::vec3 radii = npositions[0];
					float xAxisRotation = npositions[1].x;
					float largeArcFlag = std::clamp( npositions[2].x, 0.f, 1.f );
					float sweepFlag = std::clamp( npositions[2].y, 0.f, 1.f );
					
					glm::vec3 spt = prevPos;
					glm::vec3 ept = npositions[3];
					
					
//					glm::vec3 cpt(spt.x, ept.y, 0.0f);
					auto cpt = glm::vec3(findArcCenter(spt, ept, radii.x, radii.y, xAxisRotation, largeArcFlag, sweepFlag ), 0.f);
					auto windingOrder = _getWindingOrderOnArc( spt, cpt, ept );
					
					auto startDiff = glm::normalize(spt - cpt);
					auto endDiff = glm::normalize(ept - cpt);
					
					float startAngle = atan2f( startDiff.y, startDiff.x );// - glm::radians(40.f);
					float endAngle = atan2f( endDiff.y, endDiff.x );
					
					
					if( largeArcFlag < 1 ) {
						if( sweepFlag > 0 ) {
							if( windingOrder < 0 ) {
								windingOrder *= -1.f;
							}
						} else {
							if( windingOrder > 0 ) {
								windingOrder *= -1.f;
							}
						}
					} else {
						if( sweepFlag > 0 ) {
							if( windingOrder < 1 ) {
								windingOrder *= -1.f;
							}
						} else {
							if( windingOrder > -1 ) {
								windingOrder *= -1.f;
							} else {
								
							}
						}
					}
					
					startDiff = glm::normalize(spt - cpt);
					endDiff = glm::normalize(ept - cpt);
										
					startAngle = atan2f( startDiff.y, startDiff.x );// - glm::radians(40.f);
					endAngle = atan2f( endDiff.y, endDiff.x );
					
					startAngle = ofWrapRadians(startAngle);
					endAngle = ofWrapRadians(endAngle);
					
					
					std::string worderS = "co linear";
					if( windingOrder > 0 ) {
						worderS = "clockwise";
					} else if( windingOrder < 0 ) {
						worderS = "counter clockwise";
					}
					
//					ofLogNotice("Arc winding order is ") << worderS;
					
					ofPolyline tline;
					
					float xrotRad = glm::radians(xAxisRotation);
					
					if( windingOrder < 0 ) {
//						aSvgPath->path.arcNegative(cpt, radii.x, radii.y, glm::degrees(startAngle), glm::degrees(endAngle) );
						tline.arcNegative(cpt, radii.x, radii.y, glm::degrees(startAngle-xrotRad), glm::degrees(endAngle-xrotRad) );
					} else {
						tline.arc(cpt, radii.x, radii.y, glm::degrees(startAngle-xrotRad), glm::degrees(endAngle-xrotRad) );
//						aSvgPath->path.arc(cpt, radii.x, radii.y, glm::degrees(startAngle), glm::degrees(endAngle) );
					}
					
					// rotate based on x-axis rotation //
					
					for( auto& pv : tline.getVertices() ) {
						auto nv = pv - cpt;
						nv = glm::vec3( glm::rotate(glm::vec2(nv.x, nv.y), xrotRad), 0.f);
						nv += cpt;
						pv.x = nv.x;
						pv.y = nv.y;
					}
//
					// I guess we have to copy the line via commands
					if( tline.size() > 0 ) {
//						aSvgPath->path.moveTo(spt);
						for( std::size_t i = 0; i < tline.size(); i++ ) {
//							if( i == 0 ) {
//								aSvgPath->path.moveTo(tline[0]);
//							} else {
								aSvgPath->path.lineTo(tline[i]);
//							}
						}
					}
					
//					auto centers = findEllipseCenter( spt, ept, radii.x, radii.y );
					
//					ofLogNotice("centers: ") << std::get<0>(centers) << " and " << std::get<1>(centers) << " spt: " << spt << " ept: " << ept << " radii: " << radii;
					
					
					mCenterPoints.push_back(cpt);
//					mCenterPoints.push_back(cpt);
					npositions.clear();
					npositions.push_back(ept);
				} else {
					ofLogWarning("ofx::svg::Parser") << "unable to parse arc segment.";
				}
			}
			
//			mCenterPoints.push_back(currentPos);
//			mCPoints.insert( mCPoints.end(), npositions.begin(), npositions.end() );
		}
		
//		ofLogNotice(moduleName()) << "["<<cchar<<"]: " << currentString;
		
		
		justInCase++;
	}
}

//--------------------------------------------------------------
CssClass Parser::_parseStyle( ofXml& anode ) {
	CssClass css;
	
	if( mCurrentSvgCss ) {
		// apply first if we have a global style //
		for( auto& tprop : mCurrentSvgCss->properties ) {
			css.addProperty(tprop.first, tprop.second);
		}
	}
	
	// now apply all of the other via css classes //
	// now lets figure out if there is any css applied //
	if( auto classAttr = anode.getAttribute("class") ) {
		// get a list of classes, is this separated by commas?
		auto classList = ofSplitString(classAttr.getValue(), ",");
//		ofLogNotice("ofx::svg::Parser") << " going to try and parse style classes string: " << classAttr.getValue();
		for( auto& className : classList ) {
			if( mSvgCss.hasClass(className) ) {
//				ofLogNotice("ofx::svg::Parser") << " has class " << className;
				// now lets try to apply it to the path
				auto& tCss = mSvgCss.getClass(className);
				for( auto& tprop : tCss.properties ) {
					css.addProperty(tprop.first, tprop.second);
				}
			}
		}
	}
	
	// locally set on node overrides the class listing
	// are there any properties on the node?
	if( auto fillAttr = anode.getAttribute("fill")) {
		css.addProperty("fill", fillAttr.getValue());
	}
	if( auto strokeAttr = anode.getAttribute("stroke")) {
		css.addProperty("stroke", strokeAttr.getValue());
	}
	
	if( auto strokeWidthAttr = anode.getAttribute("stroke-width")) {
		css.addProperty("stroke-width", strokeWidthAttr.getValue());
	}
	
	if( auto ffattr = anode.getAttribute("font-family") ) {
		std::string tFontFam = ffattr.getValue();
		ofStringReplace( tFontFam, "'", "" );
		css.addProperty("font-family", tFontFam);
	}
	
	if( auto fsattr = anode.getAttribute("font-size") ) {
		css.addProperty("font-size", fsattr.getValue() );
	}
	
	// and lastly style
	if( auto styleAttr = anode.getAttribute("style") ) {
		css.addProperties(styleAttr.getValue());
	}
	
	// override anything else if set directly on the node
	if( auto disAttr = anode.getAttribute("display") ) {
		css.addProperties(disAttr.getValue());
	}
	
	return css;
}

//--------------------------------------------------------------
void Parser::_applyStyleToElement( ofXml& tnode, std::shared_ptr<Element> aEle ) {
	auto css = _parseStyle(tnode);
//	ofLogNotice("_applyStyleToElement" ) << " " << aEle->name << " -----";
	if( css.hasAndIsNone("display")) {
//		ofLogNotice("parser") << "setting element to invisible: " << aEle->name;
		aEle->setVisible(false);
	}
}

//--------------------------------------------------------------
void Parser::_applyStyleToPath( ofXml& tnode, std::shared_ptr<Path> aSvgPath ) {
	auto css = _parseStyle(tnode);
	_applyStyleToPath(css, aSvgPath);
}

//--------------------------------------------------------------
void Parser::_applyStyleToPath( CssClass& aclass, std::shared_ptr<Path> aSvgPath ) {
	// now lets figure out if there is any css applied //
	
	if( aclass.hasProperty("fill")) {
		if( !aclass.isNone("fill")) {
			aSvgPath->path.setFillColor(aclass.getColor("fill"));
		} else {
			aSvgPath->path.setFilled(false);
		}
	} else {
//		aSvgPath->path.setFilled(false);
		aSvgPath->path.setFillColor(ofColor(0));
	}
	
	if( !aclass.isNone("stroke") ) {
		aSvgPath->path.setStrokeColor(aclass.getColor("stroke"));
	}
	
	if( aclass.hasProperty("stroke-width")) {
		if( aclass.isNone("stroke-width")) {
			aSvgPath->path.setStrokeWidth(0.f);
		} else {
			aSvgPath->path.setStrokeWidth( aclass.getFloatValue("stroke-width", 0.f));
		}
	} else {
		// default with no value is 1.f
//		aSvgPath->path.setStrokeWidth(1.f);
	}
	
	// if the color is not set and the width is not set, then it should be 0
	if( !aclass.isNone("stroke") ) {
		if( !aclass.hasProperty("stroke-width")) {
			aSvgPath->path.setStrokeWidth(1.f);
		}
	}
}

//--------------------------------------------------------------
void Parser::_applyStyleToText( ofXml& anode, std::shared_ptr<Text::TextSpan> aTextSpan ) {
	auto css = _parseStyle(anode);
	_applyStyleToText(css, aTextSpan);
}

//--------------------------------------------------------------
void Parser::_applyStyleToText( CssClass& aclass, std::shared_ptr<Text::TextSpan> aTextSpan ) {
	// default font family
	aTextSpan->fontFamily    = aclass.getValue("font-family", "Arial");
	aTextSpan->fontSize      = aclass.getIntValue("font-size", 18 );
	aTextSpan->color 		= aclass.getColor("fill");
}
	


//--------------------------------------------------------------
glm::vec2 Parser::_parseMatrixString(const std::string& input, const std::string& aprefix ) {
//	std::string prefix = aprefix+"(";
//	std::string suffix = ")";
	ofLogVerbose(moduleName()) << __FUNCTION__ << " input: " << input;;
	std::string searchStr = aprefix + "(";
	size_t startPos = input.find(searchStr);
	
	if (startPos != std::string::npos) {
		startPos += searchStr.size();
		size_t endPos = input.find(")", startPos);
		
		if (endPos != std::string::npos) {
			// Extract the part inside the parentheses
			std::string inside = input.substr(startPos, endPos - startPos);
			
			// Ensure numbers like ".5" are correctly handled by adding a leading zero if needed
			if (inside[0] == '.') {
				inside = "0" + inside;
			}
			
			float tx = 0.f;
			float ty = 0.f;
			// Use stringstream to parse one or two numbers
			std::stringstream ss(inside);
			if (ss >> tx) {
				if (!(ss >> ty)) {
					// If only one value is provided, set Y to 0
					ty = tx;
				}
				return glm::vec2(tx,ty);
			}
		}
	}
	return glm::vec2(0.f, 0.f);
}

//--------------------------------------------------------------
bool Parser::getTransformFromSvgMatrix( string aStr, glm::vec2& apos, float& scaleX, float& scaleY, float& arotation ) {
    
    scaleX = 1.0;
    scaleY = 1.0;
    arotation = 0.0;
	//TODO: implement matrix push and pop structure, similar to renderers
	ofLogVerbose(moduleName()) << __FUNCTION__ << " going to parse string: " << aStr << " pos: " << apos;
	
	glm::mat4 mat = glm::mat4(1.f);
	
	if( ofIsStringInString(aStr, "translate")) {
		auto transStr = aStr;
		auto tp = _parseMatrixString(transStr, "translate" );
		ofLogVerbose(moduleName()) << __FUNCTION__ << " translate: " << tp;
//		apos += tp;
		mat = glm::translate(glm::mat4(1.0f), glm::vec3(tp.x, tp.y, 0.0f));
	} else {
		mat = glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.f, 0.0f));
	}
	
	if( ofIsStringInString(aStr, "rotate")) {
		auto transStr = aStr;
		auto tr = _parseMatrixString(transStr, "rotate" );
		arotation = tr.x;
		if( arotation != 0.f ) {
			mat = mat * glm::toMat4((const glm::quat&)glm::angleAxis(glm::radians(arotation), glm::vec3(0.f, 0.f, 1.f)));
		}
		ofLogVerbose(moduleName()) << __FUNCTION__ << " arotation: " << arotation;
	}
	
	if( ofIsStringInString(aStr, "scale")) {
		auto transStr = aStr;
		auto ts = _parseMatrixString(transStr, "scale" );
		scaleX = ts.x;
		scaleY = ts.y;
		ofLogVerbose(moduleName()) << __FUNCTION__ << " scale: " << ts;
		
		mat = glm::scale(mat, glm::vec3(scaleX, scaleY, 1.f));
	}
	
	glm::vec3 pos3 = mat * glm::vec4( apos.x, apos.y, 0.0f, 1.f );
	apos.x = pos3.x;
	apos.y = pos3.y;
	
	
	if( ofIsStringInString(aStr, "matrix")) {
		auto matrix = aStr;
		ofStringReplace(matrix, "matrix(", "");
		ofStringReplace(matrix, ")", "");
		vector<string> matrixNum = ofSplitString(matrix, " ", false, true);
		vector<float> matrixF;
		for(std::size_t i = 0; i < matrixNum.size(); i++){
			matrixF.push_back(ofToFloat(matrixNum[i]));
			//cout << " matrix[" << i << "] = " << matrixF[i] << " string version is " << matrixNum[i] << endl;
		}
		
		if( matrixNum.size() == 6 ) {
			
			apos.x = matrixF[4];
			apos.y = matrixF[5];
			
			scaleX = std::sqrtf(matrixF[0] * matrixF[0] + matrixF[1] * matrixF[1]) * (float)ofSign(matrixF[0]);
			scaleY = std::sqrtf(matrixF[2] * matrixF[2] + matrixF[3] * matrixF[3]) * (float)ofSign(matrixF[3]);
			
			arotation = glm::degrees( std::atan2f(matrixF[2],matrixF[3]) );
			if( scaleX < 0 && scaleY < 0 ){
				
			}else{
				arotation *= -1.0f;
			}
			//        cout << " rotation is " << arotation << endl;
			
			return true;
		}
	}
    return false;
}

//--------------------------------------------------------------
std::shared_ptr<Text::TextSpan> Parser::getTextSpanFromXmlNode( ofXml& anode ) {
	auto tspan = std::make_shared<Text::TextSpan>();;
    
    string tText = anode.getValue();
    float tx = 0;
    auto txattr = anode.getAttribute("x");
    if( txattr) {
        tx = txattr.getFloatValue();
    }
    float ty = 0;
    auto tyattr = anode.getAttribute("y");
    if( tyattr ) {
        ty = tyattr.getFloatValue();
    }
    
    tspan->text          = tText;
    tspan->rect.x        = tx;
    tspan->rect.y        = ty;
	
	_applyStyleToText(anode, tspan);
    
    return tspan;
}

//--------------------------------------------------------------
const ofRectangle Parser::getBounds(){
	return bounds;
}

//--------------------------------------------------------------
const ofRectangle Parser::getViewbox(){
	return viewbox;
}

//--------------------------------------------------------------
void Parser::drawDebug() {
//	Group::draw();
	ofSetColor( ofColor::limeGreen );
	ofNoFill();
	
//	int cindex = 0;
//	for( auto& cp : mCPoints ) {
//		ofSetColor( (float)(cindex % 2) * 255, 200, 60 );
////		ofDrawCircle( cp, (cindex+1) * 1.0f );
//		ofDrawCircle( cp, 3. );
//		cindex ++;
//	}
//	ofFill();
	
	for( std::size_t k = 0; k < mCPoints.size(); k += 3 ) {
		ofSetColor( ofColor::orange );
		ofDrawCircle( mCPoints[k+0], 6.f );
		ofSetColor( ofColor::white );
		ofDrawCircle( mCPoints[k+1], 3.f );
		ofDrawCircle( mCPoints[k+2], 3.f );
		ofDrawLine( mCPoints[k+0], mCPoints[k+1] );
		ofDrawLine( mCPoints[k+0], mCPoints[k+2] );
	}
	
	ofFill();
	
	ofSetColor( ofColor::orange );
	for( auto& cp : mCenterPoints ) {
		ofDrawCircle(cp, 4.f);
	}
	
}
