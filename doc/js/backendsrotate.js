var	backends=null;
var	bcurrent=0;
var	bopacity=1;

function backendsRotate() {
	backends=document.getElementsByClassName("backend");
	if (backends && backends.length) {
		setTimeout("backendsFadeOut()",1000);
	}
}

function backendsFadeOut() {
	bopacity=bopacity-0.1;
	if (bopacity<=0.1) {
		backends[bcurrent].style.display="none";
		bcurrent=bcurrent+1;
		if (bcurrent==backends.length) {
			bcurrent=0;
		}
		bopacity=0;
		backends[bcurrent].style.opacity=0;
		backends[bcurrent].style.display="block";
		setTimeout("backendsFadeIn()",35);
	} else {
		backends[bcurrent].style.opacity=bopacity;
		setTimeout("backendsFadeOut()",45);
	}
}

function backendsFadeIn() {
	bopacity=bopacity+0.1;
	if (bopacity>=1) {
		bopacity=1;
		backends[bcurrent].style.opacity=1;
		setTimeout("backendsFadeOut()",1000);
	} else {
		backends[bcurrent].style.opacity=bopacity;
		setTimeout("backendsFadeIn()",35);
	}
}
