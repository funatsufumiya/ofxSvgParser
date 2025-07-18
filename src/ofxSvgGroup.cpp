//
//  ofxSvgGroup.cpp
//
//  Created by Nick Hardeman on 7/31/15.
//

#include "ofxSvgGroup.h"
#include "ofGraphics.h"

using namespace ofx::svg;
using std::vector;
using std::shared_ptr;
using std::string;

//--------------------------------------------------------------
void Group::draw() {
    std::size_t numElements = mChildren.size();
    bool bTrans = pos.x != 0 || pos.y != 0.0;
    if( bTrans ) {
        ofPushMatrix();
        ofTranslate(pos.x, pos.y);
    }
    for( std::size_t i = 0; i < numElements; i++ ) {
		mChildren[i]->draw();
    }
    if( bTrans ) {
        ofPopMatrix();
    }
}

//--------------------------------------------------------------
std::size_t Group::getNumChildren() {
	return mChildren.size();
}

//--------------------------------------------------------------
vector< shared_ptr<Element> >& Group::getChildren() {
    return mChildren;
}

//--------------------------------------------------------------
vector< shared_ptr<Element> > Group::getAllChildren() {
    vector< shared_ptr<Element> > retElements;
    
    for( auto ele : mChildren ) {
        _getAllElementsRecursive( retElements, ele );
    }
    
    return retElements;
}

// flattens out hierarchy //
//--------------------------------------------------------------
void Group::_getAllElementsRecursive( vector< shared_ptr<Element> >& aElesToReturn, shared_ptr<Element> aele ) {
    if( aele ) {
        if( aele->isGroup() ) {
            shared_ptr<Group> tgroup = std::dynamic_pointer_cast<Group>(aele);
            for( auto ele : tgroup->getChildren() ) {
                _getAllElementsRecursive( aElesToReturn, ele );
            }
        } else {
            aElesToReturn.push_back( aele );
        }
    }
}

//--------------------------------------------------------------
shared_ptr<Element> Group::getElementForName( std::string aPath, bool bStrict ) {
    
    vector< std::string > tsearches;
    if( ofIsStringInString( aPath, ":" ) ) {
        tsearches = ofSplitString( aPath, ":" );
    } else {
        tsearches.push_back( aPath );
    }
    
    shared_ptr<Element> temp;
	_getElementForNameRecursive( tsearches, temp, mChildren, bStrict );
    return temp;
}

//--------------------------------------------------------------
std::vector< std::shared_ptr<Element> > Group::getChildrenForName( const std::string& aname, bool bStrict ) {
	std::vector< std::shared_ptr<Element> > relements;
	for( auto& kid : mChildren ) {
		if( bStrict ) {
			if( kid->getName() == aname ) {
				relements.push_back(kid);
			}
		} else {
			if( ofIsStringInString( kid->getName(), aname )) {
				relements.push_back(kid);
			}
		}
	}
	return relements;
}

//--------------------------------------------------------------
void Group::_getElementForNameRecursive( vector<string>& aNamesToFind, shared_ptr<Element>& aTarget, vector< shared_ptr<Element> >& aElements, bool bStrict ) {
	
	if( aNamesToFind.size() < 1 ) {
		return;
	}
	if(aTarget) {
		return;
	}
	
	bool bKeepGoing = false;
	std::string nameToFind = aNamesToFind[0];
	if( aNamesToFind.size() > 1 ) {
		if(aNamesToFind[0] == "*") {
			bKeepGoing = true;
			nameToFind = aNamesToFind[1];
		}
	}
	for( std::size_t i = 0; i < aElements.size(); i++ ) {
		bool bFound = false;
		if(bStrict) {
			if( aElements[i]->getName() == nameToFind ) {
				bFound = true;
			}
		} else {
//			std::cout << "Group::_getElementForNameRecursive: ele name: " << aElements[i]->getName() << " nameToFind: " << nameToFind << " keep going: " << bKeepGoing << std::endl;
			if( ofIsStringInString( aElements[i]->getName(), nameToFind )) {
				bFound = true;
			}
			
			if (!bFound && aElements[i]->getType() == TYPE_TEXT) {
				
				if (aElements[i]->getName() == "No Name") {
					// the ids for text block in illustrator are weird,
					// so try to grab the name from the text contents //
					auto etext = std::dynamic_pointer_cast<Text>(aElements[i]);
					if (etext) {
						if (etext->textSpans.size()) {
//                            cout << "Searching for " << aNamesToFind[0] << " in " << etext->textSpans.front().text << endl;
							if(ofIsStringInString( etext->textSpans.front()->text, aNamesToFind[0] )) {
								bFound = true;
							}
						}
					}
				}
			}
		}
		
		if( bFound && !bKeepGoing ) {
			if( !bKeepGoing && aNamesToFind.size() > 0 ) {
				aNamesToFind.erase( aNamesToFind.begin() );
			}
			if(aNamesToFind.size() == 0 ) {
				aTarget = aElements[i];
				break;
			} else {
				if( aElements[i]->getType() == TYPE_GROUP ) {
					auto tgroup = std::dynamic_pointer_cast<Group>( aElements[i] );
					_getElementForNameRecursive( aNamesToFind, aTarget, tgroup->getChildren(), bStrict );
					break;
				}
			}
		}
		
		if( bKeepGoing ) {
			if( bFound ) {
//				std::cout << "Group::_getElementForNameRecursive: SETTING TARGET: " << aElements[i]->getName() << " keep going: " << bKeepGoing << std::endl;
				aTarget = aElements[i];
				break;
			} else {
				if( aElements[i]->getType() == TYPE_GROUP ) {
//					std::cout << "Group::_getElementForNameRecursive: FOUND A GROUP, But still going: " << aElements[i]->getName() << " keep going: " << bKeepGoing << std::endl;
					auto tgroup = std::dynamic_pointer_cast<Group>( aElements[i] );
					_getElementForNameRecursive( aNamesToFind, aTarget, tgroup->getChildren(), bStrict );
				}
			}
		}
	}
	
//    for( std::size_t i = 0; i < aElements.size(); i++ ) {
//        bool bFound = false;
//        if(bStrict) {
//            if( aElements[i]->getName() == aNamesToFind[0] ) {
//                bFound = true;
//            }
//        } else {
//            if( ofIsStringInString( aElements[i]->getName(), aNamesToFind[0] )) {
////                    cout << "Found--- " << aNamesToFind[0] << endl;
//                bFound = true;
//            }
//            
//            if (!bFound && aElements[i]->getType() == TYPE_TEXT) {
//                
//                if (aElements[i]->getName() == "No Name") {
//                    // the ids for text block in illustrator are weird,
//                    // so try to grab the name from the text contents //
//                    shared_ptr<Text> etext = std::dynamic_pointer_cast<Text>(aElements[i]);
//                    if (etext) {
//                        if (etext->textSpans.size()) {
////                            cout << "Searching for " << aNamesToFind[0] << " in " << etext->textSpans.front().text << endl;
//                            if(ofIsStringInString( etext->textSpans.front()->text, aNamesToFind[0] )) {
//                                bFound = true;
//                            }
//                        }
//                    }
//                }
//            }
//        }
//        
//        if( bFound == true ) {
//            aNamesToFind.erase( aNamesToFind.begin() );
//            if( aNamesToFind.size() == 0 || aElements[i]->getType() != TYPE_GROUP ) {
//                bool bgood = false;
//                if( aElements[i] ) {
//                    bgood = true;
//                }
////                cout << "going to return one of the elements " << aNamesToFind.size() << " good: " << bgood << " " << endl;
//                aTarget = aElements[i];
//                break;
//            } else {
//                if( aElements[i]->getType() == TYPE_GROUP ) {
//                    shared_ptr<Group> tgroup = std::dynamic_pointer_cast<Group>( aElements[i] );
//                    getElementForNameRecursive( aNamesToFind, aTarget, tgroup->mChildren, bStrict );
//                    break;
//                }
//            }
//        }
//    }
}

//--------------------------------------------------------------
bool Group::replace( shared_ptr<Element> aOriginal, shared_ptr<Element> aNew ) {
    bool bReplaced = false;
    _replaceElementRecursive( aOriginal, aNew, mChildren, bReplaced );
    return bReplaced;
}

//--------------------------------------------------------------
void Group::_replaceElementRecursive( shared_ptr<Element> aTarget, shared_ptr<Element> aNew, vector< shared_ptr<Element> >& aElements, bool& aBSuccessful ) {
    for( std::size_t i = 0; i < aElements.size(); i++ ) {
        bool bFound = false;
        if( aTarget == aElements[i] ) {
            bFound = true;
            aBSuccessful = true;
            aElements[i] = aNew;
            break;
        }
        if( !bFound ) {
            if( aElements[i]->getType() == TYPE_GROUP ) {
                auto tgroup = std::dynamic_pointer_cast<Group>( aElements[i] );
                _replaceElementRecursive(aTarget, aNew, tgroup->mChildren, aBSuccessful );
            }
        }
    }
}

//--------------------------------------------------------------
string Group::toString(int nlevel) {
    
    string tstr = "";
    for( int k = 0; k < nlevel; k++ ) {
        tstr += "   ";
    }
    tstr += getTypeAsString() + " - " + getName() + "\n";
    
    if( mChildren.size() ) {
        for( std::size_t i = 0; i < mChildren.size(); i++ ) {
            tstr += mChildren[i]->toString( nlevel+1);
        }
    }
    
    return tstr;
}


//--------------------------------------------------------------
void Group::disableColors() {
    auto telements = getAllChildren();
    for( auto& ele : telements ) {
        ele->setUseShapeColor(false);
    }
}

//--------------------------------------------------------------
void Group::enableColors() {
    auto telements = getAllChildren();
    for( auto& ele : telements ) {
        ele->setUseShapeColor(true);
    }
}








