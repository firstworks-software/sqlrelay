function show(which) {
	hideAll();
	var popup=document.getElementById("popup-"+which);
	popup.style.display="inline";
}

function hideAll() {
	var popups=document.getElementsByClassName("popup");
	for (var i=0; i<popups.length; i++) {
		popups[i].style.display="none";
	}
}
