import Vue from 'vue'
import Vuex from 'vuex'

Vue.use(Vuex)

// #define STATUS_CONNECTED    0
// #define STATUS_PREVIEW      1
// #define STATUS_PROGRAM      2
// #define STATUS_LOCATE       3
// #define STATUS_CONNECTWIFI  4
// #define STATUS_CONNECTVMIX  5

export default new Vuex.Store({
  state: {
    tallys: [
      // {
      //   name: "NAME",
      //   host: "HOST",
      //   port: "123",
      //   address: "127.0.0.1",
      //   connected: true,
      //   brightness: 255,
      //   tallyNumber: 2,
      //   viewerLedEnabled: true,
      //   cameraLedEnabled: true,
      //   vmixPort: 81,
      //   vmixHost: "192.168.0.1",
      //   connection: null,
      //   ledState: 0,
      // }
    ] as any
  },
  mutations: {
    addTallys(state, tallys) {
      state.tallys = [];
      tallys.forEach((tally) => {
        console.log(tally)
        const newTally = {
          name: tally.name,
          host: tally.host,
          port: tally.port,
          address: tally.addresses[0],
          brightness: 0,
          tallyNumber: 0,
          viewerLedEnabled: false,
          cameraLedEnabled: false,
          vmixPort: 0,
          vmixHost: 0,
          connection: {},
          connectionState: 'CLOSED',
          ledState: 0
        }
        const connection = new WebSocket('ws:/' + newTally.address + ":" + newTally.port);
        newTally.connectionState = 'CONNECTING';
        connection.onmessage = function(event) {
          const data = event.data.split(":");
          switch (data[0]) {
            case "viewerLedEnabled" :
            case "cameraLedEnabled" :
              if (data[1] == "false") {
                newTally[data[0]] = data[1] = false;
              } else {
                newTally[data[0]] = true;
              }
              break;
            case "vmixPort" :
            case "tallyNumber" :
            case "brightness" :
            case "ledState":
              newTally[data[0]] = parseInt(data[1]);
              break;
            case "vmixHost":
              newTally[data[0]] = data[1];
              break;
          }
        }

        connection.onopen = function(event) {
          newTally.connectionState = 'CONNECTED';
          console.log("CONNECTED")
        }
        connection.onclose = function(event) {
          newTally.connectionState = 'CLOSED';
          console.log("CLOSED")
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
