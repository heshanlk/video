/**
 * @file
 * This file contains some useful functions which fixes different
 * Internet Explorer strange behaviours.
 * This file is inclueded at each video play.
 * See functions video_play and theme_video_get_script on video.module
 * file for more informations.
 *
 * @author Fabio Varesano <fvaresano at yahoo dot it>
 * porting to Drupal 6
 * @author Heshan Wanigasooriya <heshan at heidisoft.com><heshanmw@gmail.com>
 * @todo
*/

//We do this code only on Internet Explorer.
if (document.all) {

 // Fix for the "Click to interact with ActiveX control" message on IE
  var objects = document.getElementsByTagName( 'object' );
  for ( var i = 0; i < objects.length; i++ ) {
   var obj = objects[ i ];
   var clone = obj.cloneNode( true );
   var parent = obj .parentNode;
   var sibling = obj .nextSibling;
   parent.removeChild( obj );
   parent.insertBefore( clone, sibling );
   clone.outerHTML = clone.outerHTML;
  }


  function InsertQuicktimeVideo(vidfile, height, width)
  {
    document.writeln('<object classid="clsid:02BF25D5-8C17-4B23-BC80-D3488ABDDC6B" width="' + width + '" height="' + height + '" scale="tofit" codebase="http://www.apple.com/qtactivex/qtplugin.cab">');
    document.writeln('<param name="SRC" value="' + vidfile + '" />');
    document.writeln('<param name="AUTOPLAY" value="true" />');
    document.writeln('<param name="KIOSKMODE" value="false" />');
    document.writeln('<embed src="' + vidfile + '" width="' + width + '" height="' + height + '" scale="tofit" autoplay="true" kioskmode="false" pluginspage="http://www.apple.com/quicktime/download/"></embed>\n</object>');
  }

}



