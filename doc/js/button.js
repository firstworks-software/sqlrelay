function endsWith(str,ending) {
	return str.indexOf(ending,str.length-ending.length)!==-1;
}

function setActive(btn,img) {
	document.getElementById("button-"+btn).style.backgroundImage="url(images/"+img+")";
}

function setButton() {
	if (endsWith(document.location.pathname,"technologies.html")) {
		setActive("technologies","lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"services.html")) {
		setActive("services","lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"examples.html")) {
		setActive("examples","lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"index.html")) {
		setActive("about","lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"download.html")) {
		setActive("download","lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"license.html")) {
		setActive("license","lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"faststart.html")) {
		setActive("faststart","lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"documentation.html")) {
		setActive("documentation","lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"nativeprotocol.html")) {
		setActive("documentation","lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"support.html")) {
		setActive("support","lightbluedenim.png");
	} else if (endsWith(document.location.pathname,"successstories.html")) {
		setActive("successstories","lightbluedenim.png");
	}
}
