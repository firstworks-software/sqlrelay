var	slides=null;
var	slidebuttons=null;

var	current=0;
var	next=-1;

var	opacity=1;
var	fadeinrate=35;
var	fadeoutrate=45;

var	initialnextslidetime=8000;
var	defaultnextslidetime=8000;
var	manualnextslidetime=10000;
var	nextslidetime=defaultnextslidetime;

var	timeout=null;

function rotate() {
	slides=document.getElementsByClassName("slide");
	slidebuttons=document.getElementsByClassName("slidebutton");
	if (slides && slides.length>1) {
		timeout=setTimeout("fadeOut()",initialnextslidetime);
	}
}

function fadeOut() {
	opacity=opacity-0.1;
	if (opacity<=0.1) {
		slides[current].style.display="none";
		if (next>-1) {
			current=next;
			next=-1;
		} else {
			current=current+1;
			nextslidetime=defaultnextslidetime;
		}
		if (current==slides.length) {
			current=0;
		}
		opacity=0;
		slides[current].style.opacity=0;
		slides[current].style.display="block";
		for (var i=0; i<slidebuttons.length; i++) {
			if (i==current) {
				slidebuttons[i].src="images/denimdot.png";
			} else {
				slidebuttons[i].src="images/graydenimdot.png";
			}
		}
		timeout=setTimeout("fadeIn()",fadeinrate);
	} else {
		slides[current].style.opacity=opacity;
		timeout=setTimeout("fadeOut()",fadeoutrate);
	}
}

function fadeIn() {
	opacity=opacity+0.1;
	if (opacity>=1) {
		opacity=1;
		slides[current].style.opacity=1;
		timeout=setTimeout("fadeOut()",nextslidetime);
	} else {
		slides[current].style.opacity=opacity;
		timeout=setTimeout("fadeIn()",fadeinrate);
	}
}

function slide(which) {
	clearTimeout(timeout);
	fadeOut();
	next=which;
	nextslidetime=manualnextslidetime;
}
