function endsWith(str,ending) {
	return str.indexOf(ending,str.length-ending.length)!==-1;
}

function contains(str,part) {
	return str.includes(part);
}

function setActive(btn,img) {
	document.getElementById("button-"+btn).
			style.backgroundImage="url("+img+")";
}

function setButton() {
	if (endsWith(document.location.pathname,"index.html") ||
			endsWith(document.location.pathname,"/")) {
		setActive("about","images/lightbluedenim.png");
	} else if (contains(document.location.pathname,"/features/")) {
		setActive("about","../images/lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"documentation.html") ||
			contains(document.location.pathname,"/admin/") ||
			contains(document.location.pathname,"/api/") ||
			contains(document.location.pathname,"/howtos/") ||
			contains(document.location.pathname,"/programming/")) {
		setActive("documentation","../images/lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"faq.html")) {
		setActive("faq","images/lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"download.html")) {
		setActive("download","images/lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"license.html")) {
		setActive("license","images/lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"support.html")) {
		setActive("support","images/lightbluedenim.png");
	}
}
