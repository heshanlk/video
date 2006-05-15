function InsertQuicktimeVideo(vidfile, height, width)
{
  document.writeln('<object classid="clsid:02BF25D5-8C17-4B23-BC80-D3488ABDDC6B" width="' + width + '" height="' + height + '" scale="tofit" codebase="http://www.apple.com/qtactivex/qtplugin.cab">');
  document.writeln('<param name="SRC" value="' + vidfile + '" />');
  document.writeln('<param name="AUTOPLAY" value="true" />');
  document.writeln('<param name="KIOSKMODE" value="false" />');
  document.writeln('<embed src="' + vidfile + '" width="' + width + '" height="' + height + '" scale="tofit" autoplay="true" kioskmode="false" pluginspage="http://www.apple.com/quicktime/download/"></embed>\n</object>');
}