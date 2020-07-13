<template>
  <v-app>
    <v-app-bar
            app
            dark
    >
      <div class="d-flex align-center">
        <v-img
                alt="Tally Blaster Logo"
                class="shrink"
                contain
                src="@/assets/logo.svg"
                transition="scale-transition"
                width="100"
        />
      </div>
      <v-spacer></v-spacer>
    </v-app-bar>
    <router-view></router-view>
    <v-bottom-navigation dark>
      <v-btn to="/flash" link>
        <span>USB Flash</span>
        <v-icon>mdi-usb</v-icon>
      </v-btn>

      <v-btn to="/" link>
        <span>Tally Control</span>
        <v-icon>mdi-radar</v-icon>
      </v-btn>

<!--      <v-btn to="/settings" link>-->
<!--        <span>Settings</span>-->
<!--        <v-icon>mdi-cog</v-icon>-->
<!--      </v-btn>-->
    </v-bottom-navigation>
  </v-app>
</template>
<script lang="ts">
import Vue from 'vue';

export default Vue.extend({
  name: 'App',
});
</script>