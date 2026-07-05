"use strict";

const canvasElement = document.getElementById("canvas");
const uploadElement = document.getElementById("upload");
const primaryColourElement = document.getElementById("primary-colour");
const secondaryColourElement = document.getElementById("secondary-colour");

async function loadGame(nativeLoadGame, e) {
    const file = e.target.files[0];
    const data = new Uint8Array(await file.arrayBuffer());
    const filename = "/rom.ch8";
    FS.writeFile(filename, data);
    nativeLoadGame(filename);
}

async function refreshPalette(nativeRefreshPalette, e) {
    let primary_hex = primaryColourElement.value;
    if (primary_hex.startsWith("#")) {
        primary_hex = primary_hex.substring(1);
    }
    let secondary_hex = secondaryColourElement.value;
    if (secondary_hex.startsWith("#")) {
        secondary_hex = secondary_hex.substring(1);
    }
    nativeRefreshPalette(primary_hex, secondary_hex);
}

var Module = {
    canvas: canvasElement,
    print: (...args) => console.log(...args),
    onRuntimeInitialized() {
        console.log("On runtime initialized.");
        const nativeLoadGame = Module.cwrap("loadGame", null, ["string"]);
        const nativeRefreshPalette = Module.cwrap("refreshPalette", null, ["string", "string"]);

        uploadElement.onchange = (e) => loadGame(nativeLoadGame, e);
        primaryColourElement.onchange = (e) => refreshPalette(nativeRefreshPalette, e);
        secondaryColourElement.onchange = (e) => refreshPalette(nativeRefreshPalette, e);
    },
};
