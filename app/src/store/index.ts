import Vue from 'vue'
import Vuex from 'vuex'

Vue.use(Vuex)

// #define STATUS_CONNECTED    0
// #define STATUS_PREVIEW      1
// #define STATUS_PROGRAM      2
// #define STATUS_LOCATE       3
// #define STATUS_CONNECTWIFI  4
// #define STATUS_CONNECTVMIX  5
export interface Tally {
  name: string;
  host: string;
  port: string;
  address: string;
  brightness: number;
  tallyNumber: number;
  viewerLedEnabled: boolean;
  cameraLedEnabled: boolean;
  vmixPort: number;
  vmixHost: string;
  connection: WebSocket | null;
  connectionState: string;
  ledState: number;
  version: string;
}

export interface TallyCollection {
  tallys: Array<Tally>;
}

export default new Vuex.Store({
  state: {
    tallys: []
  } as TallyCollection,
  mutations: {
    addTallys(state, tallys) {
      state.tallys = [];
      tallys.forEach((tally) => {
        const newTally: Tally = {
          name: tally.name,
          host: tally.host,
          port: tally.port,
          address: tally.addresses[0],
          brightness: 0,
          tallyNumber: 0,
          viewerLedEnabled: false,
          cameraLedEnabled: false,
          vmixPort: 0,
          vmixHost: '',
          connection: null,
          connectionState: 'CLOSED',
          ledState: 0,
          version: 'v?'
        }
        const connection = new WebSocket('ws:/' + newTally.address + ":" + newTally.port);
        newTally.connectionState = 'CONNECTING';
        connection.onmessage = function(event) {
          const data = event.data.split(":");
          console.log(data);
          switch (data[0]) {
            case "viewerLedEnabled" :
            case "cameraLedEnabled" :
              if (data[1] == "false") {
                newTally[data[0]] = data[1] = false;
              } else {
                newTally[data[0]] = true;
              }
              break;
            case "port" :
            case "tallyNumber" :
            case "brightness" :
            case "ledState":
              newTally[data[0]] = parseInt(data[1]);
              break;
            case "sourceType":
              newTally[data[0]] = parseInt(data[1]);
              break;
            case "sourceHost":
              newTally[data[0]] = data[1];
              break;
            case "version":
              newTally[data[0]] = 'v' + data[1];
              break;
          }
        }

        connection.onopen = function() {
          newTally.connectionState = 'CONNECTED';
        }
        connection.onclose = function() {
          newTally.connectionState = 'CLOSED';
        }
        newTally.connection = connection;
        state.tallys.push(newTally)
      })
    },
  },
  actions: {
    addTallys(context, tallys) {
      context.commit('addTallys', tallys)
    }
  },
  modules: {
  },
})
