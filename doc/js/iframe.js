function sizeIFrame() {
	var	iframes=document.getElementsByTagName("iframe");
	var	existingheight=iframes[0].style.height;
	var	newheight=
	(document.getElementsByTagName("body")[0].clientHeight-141)+"px";
	if (existingheight!=newheight) {
		iframes[0].style.height=newheight;
	}
}
