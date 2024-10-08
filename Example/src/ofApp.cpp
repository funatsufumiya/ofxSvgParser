#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	
	ofSetBackgroundColor(250);
	
    ofSetFrameRate( 60 );
	svg.load("ofLogoDesserts.svg");
    ofLogNotice("Svg Structure") << std::endl << svg.toString();
	
	// get all the paths in the Sprinkles group
	auto sprinklePaths = svg.getElementsForType<ofx::svg::Path>("Donut:Sprinkles");
	
	for( auto& sprinklePath : sprinklePaths ) {
		auto spoly = sprinklePath->getFirstPolyline();
		if( spoly.size() > 2 ) {
			// set the center point
			auto centerPos = spoly.getBoundingBox().getCenter();
			sprinklePath->pos = centerPos;
			// now lets convert the paths to be local around their center point
			sprinklePath->path.clear();
			int counter = 0;
			for( auto& vert : spoly.getVertices() ) {
				vert -= centerPos;
				if( counter < 1 ) {
					sprinklePath->path.moveTo(vert);
				} else {
					sprinklePath->path.lineTo(vert);
				}
				counter++;
			}
			
			if( spoly.isClosed() ) {
				sprinklePath->path.close();
			}
			
			mSprinkles.push_back(sprinklePath);
			// don't draw using the svg::path::draw method, since it does not account for pos
			// we will draw it ourselves in the ofApp::draw()
			sprinklePath->setVisible(false);
		}
	}
}

//--------------------------------------------------------------
void ofApp::update(){
    ofColor tcolor( 101,163,253 );
    tcolor.setHue( tcolor.getHue() + (sin(ofGetElapsedTimef()*0.5)) * 50 );
	
	float etimef = ofGetElapsedTimef();
	for( auto& sprinkle : mSprinkles ) {
		
		if( ofGetMousePressed() ) {
			auto diff = glm::normalize(glm::vec2( ofGetMouseX(), ofGetMouseY() ) - glm::vec2( sprinkle->pos.x, sprinkle->pos.y ));
			float targetRotation = glm::degrees(atan2f( diff.y, diff.x ));
			sprinkle->rotation = ofLerpDegrees(sprinkle->rotation, targetRotation, 0.1f );
		} else {
			sprinkle->rotation += 2.f * ofClamp( cosf( sprinkle->pos.x * 0.1f + etimef ), -0.1f, 1.f);
		}
	}
    
}

//--------------------------------------------------------------
void ofApp::draw(){
	
	auto wrect = ofRectangle(0,0,ofGetWidth(), ofGetHeight());
	auto srect = svg.getBounds();
	srect.scaleTo(wrect, OF_SCALEMODE_FIT);
	ofPushMatrix(); {
		svg.draw();
		
		for( auto& sprinkle : mSprinkles ) {
			ofPushMatrix(); {
				ofTranslate( sprinkle->pos );
				ofRotateDeg(sprinkle->rotation);
				sprinkle->path.draw();
			} ofPopMatrix();
		}
		
		if( bDebug ) {
			svg.drawDebug();
		}
		
	} ofPopMatrix();
    
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if( key == 'd' ) {
		bDebug = !bDebug;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
