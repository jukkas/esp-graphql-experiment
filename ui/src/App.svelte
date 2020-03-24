<script>
  import client from "./graphql-client";
  import gql from "graphql-tag";
/*
  let todo = {
    title: ""
  };
  async function insertTodo() {
    const mutation = gql`
      mutation InsertTodo($title: String!) {
        insert_todo(objects: { title: $title, completed: false }) {
          affected_rows
          returning {
            id
          }
        }
      }
    `;
    client.mutate({
      mutation,
      variables: {
        title: todo.title
      }
    });
  }


*/

  async function updateLight(device) {
    const mutation = gql`
      mutation UpdateLight($serial:String!, $light:Boolean!) {
        update_devices(where: {serial: {_eq: $serial}}, _set: {light: $light}) {
          affected_rows
        }
      }
    `;
    client.mutate({
      mutation,
      variables: {
        serial: device.serial,
        light: device.light,
      }
    });
  }
  async function updateOnline(device) {
    const mutation = gql`
      mutation UpdateOnline($serial:String!, $online:Boolean!) {
        update_devices(where: {serial: {_eq: $serial}}, _set: {online: $online}) {
          affected_rows
        }
      }
    `;
    client.mutate({
      mutation,
      variables: {
        serial: device.serial,
        online: device.online,
      }
    });
  }

  function toggleLight(device) {
    device.light = !device.light;
    updateLight(device);
  }
  function toggleOnline(device) {
    device.online = !device.online;
    updateOnline(device);
  }


const query = gql`
  subscription DeviceSubscription {
    devices(order_by: {serial: asc}) {
      serial
      name
      online
      light
      temperature
      created_at
      updated_at
    }
  }  
  `;

  const devices = client.subscribe({
    query,
});
</script>

<main>
{#if $devices && $devices.data}

  <!-- <table>
    <thead><tr>
      <td>Serial</td><td>Name</td>
      <td>Online</td>
      <td>Light</td>
      <td>Temperature</td>
      <td>Created</td>
      <td>Updated</td>
    </tr></thead>
    <tbody>
      {#each $devices.data.devices as d}
        <tr>
          <td>{d.serial}</td>
          <td>{d.name}</td>
          <td>
            <label>
              <input type="checkbox" bind:checked={d.online} on:change={() => updateOnline(d)} />
              {d.online ? 'Online':'Offline'}
            </label>
          </td>
          <td>
            <label>
              <input type="checkbox" bind:checked={d.light} on:change={() => updateLight(d)} />
              {d.light ? 'On':'Off'}
            </label>
          </td>
          <td>{d.temperature}</td>
          <td>{d.created_at}</td>
          <td>{d.updated_at}</td>
        </tr>
      {/each}
    </tbody>
  </table> -->

  {#each $devices.data.devices as d}
    <section>
      <div>
        <div on:click={() => toggleOnline(d)}
              class="serial {d.online ? 'on':'off'}">
            {d.serial}
        </div>
        <div class="name">{d.name}</div>
      </div>
      <div class="light {d.light ? 'on':'off'}"
            on:click={() => toggleLight(d)}>
        {d.light ? 'On':'Off'}
      </div>
      <div>
        {d.temperature} {d.created_at} {d.updated_at}
      </div>
    </section>
  {/each}


{/if}
</main>

<style>
.serial {
  /*width: 9em;*/
  text-align: center;
}
.name {
  /*width: 20em;*/
}
.light {
  display: inline-block;
  width: 5em;
  text-align: center;
}
.on {
  background-color: lawngreen;
}
.off {
  background-color: red;
}
section {
  display: flex;
  margin: auto;
  max-width: 90%;
  border: 1px solid;
  padding: 1em;
  margin-bottom: 1em;
  border-radius: 0.5em;
}
</style>