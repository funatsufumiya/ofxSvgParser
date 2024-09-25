//
//  ofxSvgGroup.h
//
//  Created by Nick Hardeman on 7/31/15.
//

#pragma once
#include "ofxSvgElements.h"

namespace ofx::svg {
class Group : public Element {
public:
	
	virtual SvgType getType() override {return TYPE_GROUP;}
	
	virtual void draw() override;
	
	std::size_t getNumChildren();// override;
	std::vector< std::shared_ptr<Element> >& getChildren();
	std::vector< std::shared_ptr<Element> > getAllChildren();
	
	template<typename ofxSvgType>
	std::vector< std::shared_ptr<ofxSvgType> > getElementsForType( std::string aPathToGroup="", bool bStrict= false ) {
		
		std::shared_ptr<ofxSvgType> temp = std::make_shared<ofxSvgType>();
		auto sType = temp->getType();
		
		std::vector< std::shared_ptr<ofxSvgType> > telements;
		
		std::vector< std::shared_ptr<Element> > elementsToSearch;
		if( aPathToGroup == "" ) {
			elementsToSearch = mChildren;
		} else {
			std::shared_ptr< Element > temp = getElementForName( aPathToGroup, bStrict );
			if( temp ) {
				if( temp->isGroup() ) {
					std::shared_ptr< Group > tgroup = std::dynamic_pointer_cast<Group>( temp );
					elementsToSearch = tgroup->mChildren;
				}
			}
		}
		
		if( !elementsToSearch.size() && mChildren.size() ) {
			elementsToSearch = mChildren;
		}
		
		for( std::size_t i = 0; i < elementsToSearch.size(); i++ ) {
			if( elementsToSearch[i]->getType() == sType ) {
				telements.push_back( std::dynamic_pointer_cast< ofxSvgType>(elementsToSearch[i]) );
			}
		}
		return telements;
	}
	
	template<typename ofxSvgType>
	std::shared_ptr<ofxSvgType> getFirstElementForType( std::string aPathToGroup="", bool bStrict= false ) {
		auto eles = getElementsForType<ofxSvgType>(aPathToGroup, bStrict );
		if( eles.size() > 0 ) {
			return eles[0];
		}
		return std::shared_ptr<ofxSvgType>();
	}
	
	template<typename ofxSvgType>
	std::vector< std::shared_ptr<ofxSvgType> > getAllElementsForType() {
		
		auto temp = std::make_shared<ofxSvgType>();
		auto sType = temp->getType();
		
		std::vector< std::shared_ptr<ofxSvgType> > telements;
		
		auto elementsToSearch = getAllChildren();
		
		for( std::size_t i = 0; i < elementsToSearch.size(); i++ ) {
			if( elementsToSearch[i]->getType() == sType ) {
				telements.push_back( std::dynamic_pointer_cast<ofxSvgType>(elementsToSearch[i]) );
			}
		}
		return telements;
	}
	
	std::shared_ptr<Element> getElementForName( std::string aPath, bool bStrict = false );
	
	template<typename ofxSvgType>
	std::shared_ptr< ofxSvgType > get( std::string aPath, bool bStrict = false ) {
		auto stemp = std::dynamic_pointer_cast<ofxSvgType>( getElementForName( aPath, bStrict ) );
		return stemp;
	}
	
	template<typename ofxSvgType>
	std::shared_ptr< ofxSvgType > get( int aIndex ) {
		auto stemp = std::dynamic_pointer_cast<ofxSvgType>( mChildren[ aIndex ] );
		return stemp;
	}
	
	bool replace( std::shared_ptr<Element> aOriginal, std::shared_ptr<Element> aNew );
	
	virtual std::string toString(int nlevel = 0) override;
	
	void disableColors();
	void enableColors();
	
protected:
	void getElementForNameRecursive( std::vector< std::string >& aNamesToFind, std::shared_ptr<Element>& aTarget, std::vector< std::shared_ptr<Element> >& aElements, bool bStrict = false );
	void _getAllElementsRecursive( std::vector< std::shared_ptr<Element> >& aElesToReturn, std::shared_ptr<Element> aele );
	
	void _replaceElementRecursive( std::shared_ptr<Element> aTarget, std::shared_ptr<Element> aNew, std::vector< std::shared_ptr<Element> >& aElements, bool& aBSuccessful );
	
	std::vector< std::shared_ptr<Element> > mChildren;
};
}
















