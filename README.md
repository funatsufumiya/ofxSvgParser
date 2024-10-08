# ofxSvgParser
Openframeworks add-on for loading and parsing svg files.

Compatible with OpenFrameworks >= 12.0. 

The goal of this addon is to load and parse svg files while maintaining the structure of the file, ie _groups_ and _children_, etc.
Helpful for layout and placement of interactive elements or storing path information. 
The focus of this addon is placement and file structure; not rendering. So clipping, gradients and other rasterization implementations are not supported. 

![Screenshot 2024-10-07 at 10 22 41 PM](https://github.com/user-attachments/assets/cfdd4f28-a468-42a4-8c51-b4c7fbc57c09)


Grab a group by passing in the name of the group. <br/>
`shared_ptr<ofxSvgGroup> logoGroup = svg.get<ofxSvgGroup>("logo");`

Grab nested groups by passing in the hierarchy to the group separated by colons. <br/>
`shared_ptr<ofxSvgGroup> cloudGroup = svg.get<ofxSvgGroup>("sky:clouds");`

Get all of the elements of a certain type by calling getElementsForType <br/>
`vector< shared_ptr<ofxSvgImage> > trees = svg.getElementsForType<ofxSvgImage>("trees");`


Currently supports the following types:<br/>
_Group_, _Rectangle_, _Image_, _Ellipse_, _Circle_, _Path_, _Polygon_ and _Line_

Limited Support:<br/>
_Text_

The addon will attempt to load system fonts. It also searches for a folder named "fonts" in the same directory as the loaded svg file for cross platform compatability.

Text has limited support due to the breadth and complexity of the spec. <br/>

This is the successor to ofxSvgLoader (https://github.com/NickHardeman/ofxSvgLoader). <br/>
_ofxSvgParser_ has no external dependencies ( other than OF :) <br/>
Previously, _ofxSvgLoader_ depended on _ofxSvg_, which depended on _svgtiny_ to parse the paths and polygons. <br/>
This addon parses the svg document internally based on the W3 svg 2.0 spec (https://www.w3.org/TR/SVG2/Overview.html).<br/>

The svg spec is quite large and quite ambiguous at times. I have tested with several sample svg files, but please submit an issue if something is incorrect.


