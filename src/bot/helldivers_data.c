#include "bot/helldivers.h"

const char* planet_names[267] = {
    "Super Earth","Klen Dahth II","Pathfinder V","Widow's Harbor","New Haven","Pilen V","Hydrofall Prime","Zea Rugosia","Darrowsport","Fornskogur II",
    "Midasburg","Cerberus IIIc","Prosperity Falls","Okul VI","Martyr's Bay","Freedom Peak","Fort Union","Kelvinor","Wraith","Igla","New Kiruna",
    "Fort Justice","Zegema Paradise","Providence","Primordia","Sulfura","Nublaria I","Krakatwo","Volterra","Crucible","Veil","Marre IV","Fort Sanctuary",
    "Seyshel Beach","Hellmire","Effluvia","Solghast","Diluvia","Viridia Prime","Obari","Myradesh","Atrama","Emeria","Barabos","Fenmire","Mastia",
    "Shallus","Krakabos","Iridica","Azterra","Azur Secundus","Ivis","Slif","Caramoor","Kharst","Eukoria","Myrium","Kerth Secundus","Parsh","Reaf","Irulta",
    "Emorath","Ilduna Prime","Maw","Meridia","Borea","Curia","Tarsh","Shelt","Imber","Blistica","Ratch","Julheim","Valgaard","Arkturus","Esker","Terrek",
    "Cirrus","Crimsica","Heeth","Veld","Alta V","Ursica XI","Inari","Skaash","Moradesh","Rasp","Bashyr","Regnus","Mog","Valmox","Iro","Grafmere",
    "New Stockholm","Oasis","Genesis Prime","Outpost 32","Calypso","Elysian Meadows","Alderidge Cove","Trandor","East Iridium Trading Bay","Liberty Ridge",
    "Baldrick Prime","The Weir","Kuper","Oslo Station","Pöpli IX","Gunvald","Dolph","Bekvam III","Duma Tyr","Vernen Wells","Aesir Pass","Aurora Bay","Penta",
    "Gaellivare","Vog-sojoth","Kirrik","Mortax Prime","Wilford Station","Pioneer II","Erson Sands","Socorro III","Bore Rock","Fenrir III","Turing",
    "Angel's Venture","Darius II","Acamar IV","Achernar Secundus","Achird III","Acrab XI","Acrux IX","Acubens Prime","Adhara","Afoyay Bay","Ain-5",
    "Alairt III","Alamak VII","Alaraph","Alathfar XI","Andar","Asperoth Prime","Bellatrix","Botein","Osupsam","Brink-2","Bunda Secundus","Canopus","Caph",
    "Castor","Durgen","Draupnir","Mort","Ingmar","Charbal-VII","Charon Prime","Choepessa IV","Choohe","Chort Bay","Claorell","Clasa","Demiurg","Deneb Secundus",
    "Electra Bay","Enuliale","Epsilon Phoencis VI","Erata Prime","Estanu","Fori Prime","Gacrux","Gar Haren","Gatria","Gemma","Grand Errant","Hadar","Haka",
    "Haldus","Halies Port","Herthon Secundus","Hesoe Prime","Heze Bay","Hort","Hydrobius","Karlia","Keid","Khandark","Klaka 5","Kneth Port","Kraz","Kuma",
    "Lastofe","Leng Secundus","Lesath","Maia","Malevelon Creek","Mantes","Marfark","Martale","Matar Bay","Meissa","Mekbuda","Menkent","Merak","Merga IV",
    "Minchir","Mintoria","Mordia 9","Nabatea Secundus","Navi VII","Nivel 43","Oshaune","Overgoe Prime","Pandion-XXIV","Partion","Peacock","Phact Bay",
    "Pherkad Secundus","Polaris Prime","Pollux 31","Prasa","Propus","Ras Algethi","Rd-4","Rogue 5","Rirga Bay","Seasse","Senge 23","Setia","Shete","Siemnot",
    "Sirius","Skat Bay","Spherion","Stor Tha Prime","Stout","Termadon","Tibit","Tien Kwan","Troost","Ubanea","Ustotu","Vandalon IV","Varylia 5","Wasat","Vega Bay",
    "Wezen","Vindemitarix Prime","X-45","Yed Prior","Zefia","Zosma","Zzaniah Prime","Skitter","Euphoria III","Diaspora X","Gemstone Bluffs","Zagon Prime",
    "Omicron","Cyberstan","Fury","K",       "Null","Null","Null",       "Mox"
};


const char* factions[4] = {"Super Earth", "Terminids", "Automatons", "Illuminate"};


const char* cyberstan_names[8] = {
    "Transcendence", "Autonomy", "Star Kield", "Omniparitus",
    "Solidaritet", "Ursoot Nine", "Lurza", "Camrat For"
};
const char* cyberstan_classes[8] = {
    "Cyborg Capital", "Class 1", "Class 3", "Class 2",
    "Class 1", "Class 3", "Class 2", "Class 2"
};
const int cyberstan_max[8] = {2000000, 1000000, 1500000, 1250000, 1000000, 1500000, 1250000, 1250000};