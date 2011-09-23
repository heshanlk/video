/** 
* Copyright 2008-2009 massimocorner.com
* @author      Massimo Foti (massimo@massimocorner.com)
* @version     0.5.3, 2009-03-18
* @require     jquery.js
* @require     jquery-ui-modalframe.js
*/

if (typeof(tmt) == "undefined"){
	tmt = {};
}
	
if (typeof(tmt.jquery) == "undefined"){
	tmt.jquery = {};
}

// Child document, Look for the outermost document and point to it
if (top.tmt && top.tmt.jquery && top.tmt.jquery.modalframe){
	tmt.jquery.modalframe = top.tmt.jquery.modalframe;
}
// We are the first document in the chain
else {
	if (typeof(tmt.jquery.modalframe) == "undefined"){
		tmt.jquery.modalframe = {};
	}
	
	// Stack holding the currently opened frames
	tmt.jquery.modalframe.stack = [];
	// Counter
	tmt.jquery.modalframe.indexCounter = 1;
	
	tmt.jquery.isLegacyBrowser = false;
	if((jQuery.browser.msie) && (jQuery.browser.version < 7)){
		tmt.jquery.isLegacyBrowser = true;
	}
	
	/**
	* Open a frame
	*/
	tmt.jquery.modalframe.open = function(url, options){
		if(tmt.jquery.isLegacyBrowser){
			var args = options ? options : {};
			var heightArg = (args.height == null) ? 200 : args.height;
			var widthArg = (args.width == null) ? 300 : args.width;
			var newWin = window.open(url, "modalframe" , "location=0,status=0,scrollbars=1,resizable=0,width=" + widthArg + ",height=" + heightArg);
			newWin.focus();
		}
		else{
			tmt.jquery.modalframe.stack.push(tmt.jquery.modalframe.getInstance(url, options));
		}	
	}

	/**
	* Close the topmost frame, if any
	*/
	tmt.jquery.modalframe.close = function(){
		var currentDialog = tmt.jquery.modalframe.getTopFrame();
		if(currentDialog){
			currentDialog.remove();
			tmt.jquery.modalframe.stack.pop();
		}
	}

	/**
	* Close all frames
	*/
	tmt.jquery.modalframe.closeAll = function(){
		while(tmt.jquery.modalframe.stack.length != 0){
			tmt.jquery.modalframe.close();
		}
	}

	/**
	* Return the opener window of the topmost frame
	*/
	tmt.jquery.modalframe.getOpener = function(){
		if(tmt.jquery.modalframe.stack.length > 1){
			// Opened by another dialog
			var previousDialog = tmt.jquery.modalframe.stack[tmt.jquery.modalframe.stack.length -2];
			return previousDialog.getWindow();
		}
		else{
			return top;
		}
		return null;
	}

	/**
	* Refresh the opener window of the topmost frame
	*/
	tmt.jquery.modalframe.refreshOpener = function(){
		var opener = tmt.jquery.modalframe.getOpener();
		if(opener){
			opener.location.reload();
		}
	}

	/**
	* Resize the topmost frame
	*/
	tmt.jquery.modalframe.resize = function(newWidth, newHeight){
		var currentDialog = tmt.jquery.modalframe.getTopFrame();
		if(currentDialog){
			currentDialog.resize(newWidth, newHeight);
		}
	}

	/**
	* Privare method, return the topmost widget, null if no widget
	*/
	tmt.jquery.modalframe.getTopFrame = function(newWidth, newHeight){
		if(tmt.jquery.modalframe.stack.length > 0){
			return tmt.jquery.modalframe.stack[tmt.jquery.modalframe.stack.length -1];
		}
		return null;
	}

	/**
	* Private factory
	*/	
	tmt.jquery.modalframe.getInstance = function(url, options){
		var args = options ? options : {};
		var titleArg = (args.title == null) ? "" : args.title;
		var resizableArg = (args.resizable == null) ? false : args.resizable;
		var positionArg = (args.position == null) ? "center" : args.position;
		var heightArg = (args.height == null) ? 200 : args.height;
		var widthArg = (args.width == null) ? 300 : args.width;
		var opacityArg = (args.opacity == null) ? 0.5 : args.opacity;
		var backgroundArg = (args.background == null) ? "#000000" : args.background;
		var nextIndex = tmt.jquery.modalframe.indexCounter +1;
		tmt.jquery.modalframe.indexCounter ++;
		var frameName = "tmtModalFrame" + nextIndex;
		var retObj = {};
		var iframeNode = jQuery('<iframe id="' + frameName + '" name="' + frameName + '" class="tmtModalFrame" style="display: none" src="' + url + '"></iframe>');
		iframeNode.appendTo("body");
		var dialogObj = iframeNode.dialog({	
			title: titleArg,
			modal: true,
			resizable: resizableArg,
			position: positionArg,
			height: heightArg,
			width: widthArg,
			overlay: {
				opacity: opacityArg,
				background: backgroundArg
			},
			close: tmt.jquery.modalframe.close,
			open: function(){
				if(args.onOpen){
					args.onOpen.call();
				}
			}
		});
		iframeNode.show();
		retObj.iframe = iframeNode;
		retObj.dialog = dialogObj;
		var dialogContainer = document.getElementById(frameName).parentNode.parentNode;

		retObj.remove = function(){
			if(args.onClose){
				args.onClose.call();
			}
			jQuery(iframeNode).dialog("destroy");
			jQuery(iframeNode).remove();
		}

		retObj.resize = function(newWidth, newHeight){
			jQuery(dialogContainer).width(newWidth);
			jQuery(iframeNode).width(newWidth);
			jQuery(dialogContainer).height(newHeight);
			jQuery(iframeNode).height(newHeight);
		}

		retObj.getWindow = function(){
			return frames[frameName];
		}

		return retObj;
	}
}
